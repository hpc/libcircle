/**
 * @file
 *
 * The abstraction of a worker process.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <mpi.h>

#include "log.h"
#include "libcircle.h"
#include "token.h"
#include "lib.h"
#include "worker.h"

CIRCLE_handle queue_handle;

extern CIRCLE_input_st CIRCLE_INPUT_ST;
int8_t CIRCLE_ABORT_FLAG = 0;

/*
 * Define as described in gethostent(3) for backwards compatibility.
 */
#ifndef h_addr
#define h_addr h_addr_list[0]
#endif /* h_addr */

/**
 * @brief Function to be called in the event of an MPI error.
 *
 * This function get registered with MPI to be called
 * in the event of an MPI Error.  It attempts
 * to checkpoint.
 */
#pragma GCC diagnostic ignored "-Wunused-parameter"
static void CIRCLE_MPI_error_handler(MPI_Comm* comm, int* err, ...)
{
    char name[MPI_MAX_OBJECT_NAME];
    int namelen;
    MPI_Comm_get_name(*comm, name, &namelen);

    if(*err == LIBCIRCLE_MPI_ERROR) {
        LOG(CIRCLE_LOG_ERR, "Libcircle received abort signal, checkpointing.");
    }
    else {
        char error[MPI_MAX_ERROR_STRING];
        int error_len = 0;
        MPI_Error_string(*err, error, &error_len);
        LOG(CIRCLE_LOG_ERR, "MPI Error in Comm [%s]: %s", name, error);
        LOG(CIRCLE_LOG_ERR, "Libcircle received MPI error, checkpointing.");
    }

    CIRCLE_checkpoint();
    exit(EXIT_FAILURE);
}
#pragma GCC diagnostic warning "-Wunused-parameter"

/**
 * Wrapper for pushing an element on the queue
 *
 */
static int8_t CIRCLE_enqueue(char* element)
{
    return CIRCLE_internal_queue_push(CIRCLE_INPUT_ST.queue, element);
}

/**
 * Wrapper for popping an element
 */
static int8_t CIRCLE_dequeue(char* element)
{
    return CIRCLE_internal_queue_pop(CIRCLE_INPUT_ST.queue, element);
}

/**
 * Wrapper for getting the local queue size
 */
static uint32_t CIRCLE_local_queue_size(void)
{
    return (uint32_t)CIRCLE_INPUT_ST.queue->count;
}

/**
 * Wrapper for reading in restart files
 */
int8_t _CIRCLE_read_restarts(void)
{
    return CIRCLE_internal_queue_read(CIRCLE_INPUT_ST.queue, \
                                      CIRCLE_global_rank);
}

/**
 * Wrapper for checkpointing
 */
int8_t _CIRCLE_checkpoint(void)
{
    return CIRCLE_internal_queue_write(CIRCLE_INPUT_ST.queue, \
                                       CIRCLE_global_rank);
}

/**
 * Initializes all variables local to a rank
 */
static void CIRCLE_init_local_state(MPI_Comm comm, CIRCLE_state_st* local_state)
{
    /* get our rank and number of ranks in communicator */
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* set rank and size in state */
    local_state->comm = CIRCLE_INPUT_ST.comm;
    local_state->rank = rank;
    local_state->size = size;

    /* start the termination token on rank 0 */
    local_state->token_is_local = 0;

    if(rank == 0) {
        local_state->token_is_local = 1;
    }

    /* identify ranks for the token ring */
    local_state->token_src  = (rank - 1 + size) % size;
    local_state->token_dest = (rank + 1 + size) % size;

    /* initialize token state */
    local_state->token_proc     = WHITE;
    local_state->token_buf      = BLACK;
    local_state->token_send_req = MPI_REQUEST_NULL;

    /* allocate memory for our offset arrays */
    int32_t offsets = CIRCLE_INPUT_ST.queue->str_count;
    local_state->offsets_count = offsets;
    local_state->offsets_send_buf = (int*) calloc((size_t)offsets, sizeof(int));
    local_state->offsets_recv_buf = (int*) calloc((size_t)offsets, sizeof(int));

    /* allocate array for work request */
    size_t array_elems = (size_t) size;
    local_state->requestors = (int*) malloc(sizeof(int) * array_elems);

    /* randomize the first task we request work from */
    local_state->seed = (unsigned) rank;
    CIRCLE_get_next_proc(local_state);

    /* initialize work request state */
    local_state->work_requested = 0;

    /* determine whether we are using tree-based or circle-based
     * termination detection */
    local_state->term_tree_enabled = 0;
    if(CIRCLE_INPUT_ST.options & CIRCLE_TERM_TREE) {
        local_state->term_tree_enabled = 1;
    }

    /* create our collective tree */
    int tree_width = CIRCLE_INPUT_ST.tree_width;
    CIRCLE_tree_init(rank, size, tree_width, local_state->comm, &local_state->tree);

    /* init state for progress reduction operations */
    local_state->reduce_enabled = 0;
    double secs = (double) CIRCLE_INPUT_ST.reduce_period;
    if(secs > 0.0) {
        local_state->reduce_enabled = 1;
    }
    local_state->reduce_time_last     = MPI_Wtime();
    local_state->reduce_time_interval = secs;
    local_state->reduce_outstanding   = 0;

    /* init state for cleanup barrier operations */
    local_state->barrier_started = 0;
    local_state->barrier_up      = 0;
    local_state->barrier_replies = 0;

    /* init state for termination allreduce operations */
    local_state->work_outstanding = 0;
    local_state->term_flag    = 1;
    local_state->term_up      = 0;
    local_state->term_replies = 0;

    /* init state for abort broadcast tree */
    local_state->abort_state       = 0;
    local_state->abort_outstanding = 0;

    /* compute number of MPI requets we'll use in abort
     * (parent + num_children)*2 for one isend/irecv each */
    int num_req = local_state->tree.children;
    if(local_state->tree.parent_rank != MPI_PROC_NULL) {
        num_req++;
    }
    num_req *= 2;

    local_state->abort_num_req = num_req;
    local_state->abort_req     = (MPI_Request*) calloc((size_t)num_req, sizeof(MPI_Request));

    int i;
    for(i = 0; i < num_req; i++) {
        local_state->abort_req[i] = MPI_REQUEST_NULL;
    }

    /* initalize counters */
    local_state->local_objects_processed = 0;
    local_state->local_work_requested    = 0;
    local_state->local_no_work_received  = 0;

    return;
}

/* provides address of pointer, and if value of pointer is not NULL,
 * frees memory and sets pointer value to NULL */
void CIRCLE_free(void* pptr)
{
    void** ptr = (void**) pptr;

    if(ptr != NULL) {
        if(*ptr != NULL) {
            free(*ptr);
            *ptr = NULL;
        }
    }

    return;
}

/**
 * Free memory associated with state
 */
static void CIRCLE_finalize_local_state(CIRCLE_state_st* local_state)
{
    CIRCLE_tree_free(&local_state->tree);
    CIRCLE_free(&local_state->abort_req);
    CIRCLE_free(&local_state->offsets_send_buf);
    CIRCLE_free(&local_state->offsets_recv_buf);
    CIRCLE_free(&local_state->requestors);
    return;
}

/**
 * @brief Function that actually does work, calls user callback.
 *
 * This is the main body of execution.
 *
 * - For every work loop execution, the following happens:
 *     -# Check for work requests from other ranks.
 *     -# If this rank doesn't have work, ask a random rank for work.
 *     -# If this rank has work, call the user callback function.
 *     -# If after requesting work, this rank still doesn't have any,
 *        check for termination conditions.
 */
static void CIRCLE_work_loop(CIRCLE_state_st* sptr, CIRCLE_handle* q_handle)
{
    int cleanup = 0;

    /* Loop until done, we break on normal termination or abort */
    while(1) {
        /* Check for and service work requests */
        CIRCLE_workreq_check(CIRCLE_INPUT_ST.queue, sptr, cleanup);

        /* process any incoming work receipt messages */
        CIRCLE_workreceipt_check(CIRCLE_INPUT_ST.queue, sptr);

        /* check for incoming abort messages */
        CIRCLE_abort_check(sptr, cleanup);

        /* Make progress on any outstanding reduction */
        if(sptr->reduce_enabled) {
            CIRCLE_reduce_check(sptr, sptr->local_objects_processed, cleanup);
        }

        /* If I have no work, request work from another rank */
        if(CIRCLE_INPUT_ST.queue->count == 0) {
            CIRCLE_request_work(CIRCLE_INPUT_ST.queue, sptr, cleanup);
        }

        /* If I have some work and have not received a signal to
         * abort, process one work item */
        if(CIRCLE_INPUT_ST.queue->count > 0 && !CIRCLE_ABORT_FLAG) {
            (*(CIRCLE_INPUT_ST.process_cb))(q_handle);
            sptr->local_objects_processed++;
        }
        /* If I don't have work, or if I received signal to abort,
         * check for termination */
        else {
            /* check whether we have terminated */
            int term_status;
            if (sptr->term_tree_enabled) {
                term_status = CIRCLE_check_for_term_allreduce(sptr);
            }
            else {
                term_status = CIRCLE_check_for_term(sptr);
            }

            if(term_status == TERMINATE) {
                /* got the terminate signal, break the loop */
                LOG(CIRCLE_LOG_DBG, "Received termination signal.");
                break;
            }
        }
    }

    /* We get here either because all procs have completed all work,
     * or we got an ABORT message. */

    /* We need to be sure that all sent messages have been received
     * before returning control to the user, so that if the caller
     * invokes libcirlce again, no messages from this invocation
     * interfere with messages from the next.  Since receivers do not
     * know wether a message is coming to them, a sender records whether
     * it has a message outstanding.  When a receiver gets a message,
     * it acknowledges receipt by sending a reply back to the sender,
     * and at that point, the sender knows its message is no longer
     * outstanding.  All processes continue to receive and reply to
     * incoming messages until all senders have declared that they
     * have no more outstanding messages. */

    /* To get here when using the termination allreduce, we can be sure
     * that there is both no outstanding work transfer message for this
     * process nor any additional termination allreduce messages to
     * cleanup.  This was not immediately obvious, so here is a proof
     * to convince myself:
     *
     * Assumptions (requirements):
     * a) A process only makes progress on the current termination
     *    reduction when its queue is empty or it is in abort state.
     * b) If a process is in abort state, it does not transfer work.
     * c) If a process transferred work at any point before sending
     *    to its parent, it will force a new reduction iteration after
     *    the work has been transferred to the new process by setting
     *    its reduction flag to 0.
     *
     * Question:
     * - Can a process have an outstanding work transfer at the point
     *   it receives 1 from the termination allreduce?
     *
     * Answer: No (why?)
     * In order to send to its parent, this process (X) must have an
     * empty queue or is in the abort state.  If it transferred data
     * before calling the reduction, it would set its flag=0 and
     * force a new reduction iteration.  So to get a 1 from the
     * reduction it must not have transferred data before sending to
     * its parent.  If the process was is abort state, it cannot have
     * transferred work after sending to its parent, so it can only be
     * that its queue must have been empty, then it sent flag=1 to its
     * parent, and then transferred work.
     *
     * For its queue to have been empty and then for it to have
     * transferred work later, it must have received work items from
     * another process (Y).  If that sending process (Y) was from a
     * process yet to contribute its flag to the allreduce, the current
     * iteration would return 0, so it must be from a process that had
     * already contributed a 1.  For that process (Y) to have sent 1 to
     * its own parent, it must have an empty queue or be in the abort
     * state.  If it was in abort state, it could not have transferred
     * work, so it must have had an empty queue, meaning that it must
     * have acquired the work items from another process (Z) and
     * process Z cannot be either process X or Y.
     *
     * We can apply this logic recursively until we rule out all
     * processes in the tree, i.e., no process yet to send to its
     * parent and no process that has already sent a 1 to its parent,
     * and of course if any process had sent 0 to its parent, then
     * the allreduce will not return 1 on that iteration.
     *
     * Thus, there is no need to cleanup work transfer or termination
     * allreduce messages at this point. */

    cleanup = 1;

    /* clear up any MPI messages that may still be outstanding */
    while(1) {
        /* start a non-blocking barrier once we have no outstanding
         * items */
        if(! sptr->work_requested     &&
           ! sptr->reduce_outstanding &&
           ! sptr->abort_outstanding  &&
           sptr->token_send_req == MPI_REQUEST_NULL)
        {
            CIRCLE_barrier_start(sptr);
        }

        /* break the loop when the non-blocking barrier completes */
        if(CIRCLE_barrier_test(sptr)) {
            break;
        }

        /* send no work message for any work request that comes in */
        CIRCLE_workreq_check(CIRCLE_INPUT_ST.queue, sptr, cleanup);

        /* cleanup any outstanding reduction */
        if(sptr->reduce_enabled) {
            CIRCLE_reduce_check(sptr, sptr->local_objects_processed, cleanup);
        }

        /* receive any incoming work reply messages */
        CIRCLE_request_work(CIRCLE_INPUT_ST.queue, sptr, cleanup);

        /* drain any outstanding abort messages */
        CIRCLE_abort_check(sptr, cleanup);

        /* if we're using circle-based token passing, drain any
         * messages still outstanding */
        if(! sptr->term_tree_enabled) {
            /* check for and receive any incoming token */
            CIRCLE_token_check(sptr);

            /* if we have an outstanding token, check whether it's been received */
            if(sptr->token_send_req != MPI_REQUEST_NULL) {
                int flag;
                MPI_Status status;
                MPI_Test(&sptr->token_send_req, &flag, &status);
            }
        }
    }

    /* execute final, synchronous reduction if enabled, this ensures
     * that we fire at least one reduce and one with the final result */
    if(sptr->reduce_enabled) {
        CIRCLE_reduce_sync(sptr, sptr->local_objects_processed);
    }

    /* if any process is in the abort state,
     * set all to be in the abort state */
    CIRCLE_abort_reduce(sptr);

    return;
}

/**
 * @brief Sets up libcircle, calls work loop function
 *
 * - Main worker function. This function:
 *     -# Initializes MPI
 *     -# Initializes internal libcircle data structures
 *     -# Calls libcircle's main work loop function.
 *     -# Checkpoints if CIRCLE_abort has been called by a rank.
 */
int8_t CIRCLE_worker()
{
    /* Holds all worker state */
    CIRCLE_state_st local_state;
    CIRCLE_state_st* sptr = &local_state;

    /* Provides an interface to the queue. */
    queue_handle.enqueue = &CIRCLE_enqueue;
    queue_handle.dequeue = &CIRCLE_dequeue;
    queue_handle.local_queue_size = &CIRCLE_local_queue_size;

    /* get MPI communicator */
    MPI_Comm comm = CIRCLE_INPUT_ST.comm;

    /* get our rank and the size of the communicator */
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* initialize all local state variables */
    CIRCLE_init_local_state(comm, sptr);

    /* setup an MPI error handler */
    MPI_Errhandler circle_err;
    MPI_Comm_create_errhandler(CIRCLE_MPI_error_handler, &circle_err);
    MPI_Comm_set_errhandler(comm, circle_err);

    /* print settings of some runtime tunables */
    if(CIRCLE_INPUT_ST.options & CIRCLE_SPLIT_EQUAL) {
        LOG(CIRCLE_LOG_DBG, "Using equalized load splitting.");
    }

    if(CIRCLE_INPUT_ST.options & CIRCLE_SPLIT_RANDOM) {
        LOG(CIRCLE_LOG_DBG, "Using randomized load splitting.");
    }

    if(CIRCLE_INPUT_ST.options & CIRCLE_CREATE_GLOBAL) {
        LOG(CIRCLE_LOG_DBG, "Create callback enabled on all ranks.");
    }
    else {
        LOG(CIRCLE_LOG_DBG, "Create callback enabled on rank 0 only.");
    }

    if(CIRCLE_INPUT_ST.options & CIRCLE_TERM_TREE) {
        LOG(CIRCLE_LOG_DBG, "Using tree termination detection.");
    }
    else {
        LOG(CIRCLE_LOG_DBG, "Using circle termination detection.");
    }

    LOG(CIRCLE_LOG_DBG, "Tree width: %d", CIRCLE_INPUT_ST.tree_width);
    LOG(CIRCLE_LOG_DBG, "Reduce period (secs): %d", CIRCLE_INPUT_ST.reduce_period);

    /**********************************
     * this is where the heavy lifting is done
     **********************************/

    /* add initial work to queues by calling create_cb,
     * only invoke on master unless CREATE_GLOBAL is set */
    if(rank == 0 || CIRCLE_INPUT_ST.options & CIRCLE_CREATE_GLOBAL) {
        (*(CIRCLE_INPUT_ST.create_cb))(&queue_handle);
    }

    /* work until we get a terminate message */
    CIRCLE_work_loop(sptr, &queue_handle);

    /* we may have dropped out early from an abort signal,
     * in which case we should checkpoint here */
    if(CIRCLE_ABORT_FLAG) {
        CIRCLE_checkpoint();
    }

    /**********************************
     * end work
     **********************************/

    /* optionally print summary info */
    if (CIRCLE_debug_level >= CIRCLE_LOG_INFO) {
        /* allocate memory for summary data */
        size_t array_elems = (size_t) size;
        uint32_t* total_objects_processed_array = (uint32_t*) calloc(array_elems, sizeof(uint32_t));
        uint32_t* total_work_requests_array = (uint32_t*) calloc(array_elems, sizeof(uint32_t));
        uint32_t* total_no_work_received_array = (uint32_t*) calloc(array_elems, sizeof(uint32_t));

        /* gather and reduce summary info */
        MPI_Gather(&sptr->local_objects_processed,    1, MPI_INT,
                   &total_objects_processed_array[0], 1, MPI_INT,
                   0, comm);
        MPI_Gather(&sptr->local_work_requested,   1, MPI_INT,
                   &total_work_requests_array[0], 1, MPI_INT,
                   0, comm);
        MPI_Gather(&sptr->local_no_work_received,    1, MPI_INT,
                   &total_no_work_received_array[0], 1, MPI_INT,
                   0, comm);

        int total_objects_processed = 0;
        MPI_Reduce(&sptr->local_objects_processed, &total_objects_processed, 1,
                   MPI_INT, MPI_SUM, 0, comm);

        /* print summary from rank 0 */
        if(rank == 0) {
            int i;
            for(i = 0; i < size; i++) {
                LOG(CIRCLE_LOG_INFO, "Rank %d\tObjects Processed %d\t%0.3lf%%", i,
                    total_objects_processed_array[i],
                    (double)total_objects_processed_array[i] /
                    (double)total_objects_processed * 100.0);
                LOG(CIRCLE_LOG_INFO, "Rank %d\tWork requests: %d", i, total_work_requests_array[i]);
                LOG(CIRCLE_LOG_INFO, "Rank %d\tNo work replies: %d", i, total_no_work_received_array[i]);
            }

            LOG(CIRCLE_LOG_INFO,
                "Total Objects Processed: %d", total_objects_processed);
        }

        /* free memory */
        CIRCLE_free(&total_no_work_received_array);
        CIRCLE_free(&total_work_requests_array);
        CIRCLE_free(&total_objects_processed_array);
    }

    /* restore original error handler and free our custom one */
    MPI_Comm_set_errhandler(comm, MPI_ERRORS_ARE_FATAL);
    MPI_Errhandler_free(&circle_err);

    /* free memory */
    CIRCLE_finalize_local_state(sptr);

    return 0;
}

/* EOF */

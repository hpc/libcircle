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

    /* create our collective tree */
    CIRCLE_tree_init(rank, size, 2, local_state->comm, &local_state->tree);

    /* init state for reduction operations */
    local_state->reduce_enabled       = 1;    /* hard code to always for now */
    local_state->reduce_time_last     = MPI_Wtime();
    local_state->reduce_time_interval = 10.0; /* hard code to 10 seconds for now */
    local_state->reduce_outstanding   = 0;

    /* init state for barrier operations */
    local_state->barrier_started = 0;
    local_state->barrier_up      = 0;
    local_state->barrier_replies = 0;

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
            int term_status = CIRCLE_check_for_term(sptr);

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

    cleanup = 1;

    /* clear up any MPI messages that may still be outstanding */
    while(1) {
        /* start a non-blocking barrier once we have no outstanding
         * items */
        if(! sptr->work_requested &&
                ! sptr->reduce_outstanding &&
                sptr->token_send_req == MPI_REQUEST_NULL) {
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

        /* check for and receive any incoming token */
        CIRCLE_token_check(sptr);

        /* if we have an outstanding token, check whether it's been received */
        if(sptr->token_send_req != MPI_REQUEST_NULL) {
            int flag;
            MPI_Status status;
            MPI_Test(&sptr->token_send_req, &flag, &status);
        }
    }

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

    /* allocate memory for summary data later */
    size_t array_elems = (size_t) size;
    uint32_t* total_objects_processed_array = (uint32_t*) calloc(array_elems, sizeof(uint32_t));
    uint32_t* total_work_requests_array = (uint32_t*) calloc(array_elems, sizeof(uint32_t));
    uint32_t* total_no_work_received_array = (uint32_t*) calloc(array_elems, sizeof(uint32_t));

    /* print settings of some runtime tunables */
    if(CIRCLE_INPUT_ST.options & CIRCLE_SPLIT_EQUAL) {
        LOG(CIRCLE_LOG_DBG, "Using equalized load splitting.");
    }

    if(CIRCLE_INPUT_ST.options & CIRCLE_SPLIT_RANDOM) {
        LOG(CIRCLE_LOG_DBG, "Using randomized load splitting.");
    }

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

    /* restore original error handler and free our custom one */
    MPI_Comm_set_errhandler(comm, MPI_ERRORS_ARE_FATAL);
    MPI_Errhandler_free(&circle_err);

    /* free memory */
    CIRCLE_free(&total_no_work_received_array);
    CIRCLE_free(&total_work_requests_array);
    CIRCLE_free(&total_objects_processed_array);
    CIRCLE_finalize_local_state(sptr);

    return 0;
}

/* EOF */

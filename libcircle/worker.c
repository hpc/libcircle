/**
 * @file
 *
 * The abstraction of a worker process.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <mpi.h>

#include "log.h"
#include "libcircle.h"
#include "token.h"
#include "lib.h"
#include "worker.h"

extern CIRCLE_input_st CIRCLE_INPUT_ST;
int32_t local_objects_processed = 0;
int32_t total_objects_processed = 0;

int8_t CIRCLE_ABORT_FLAG = 0;

/**
 * @brief Function to be called in the event of an MPI error.
 *
 * This function get registered with MPI to be called
 * in the event of an MPI Error.  It attempts
 * to checkpoint.
 */
#pragma GCC diagnostic ignored "-Wunused-parameter"
void CIRCLE_MPI_error_handler(MPI_Comm* comm, int* err, ...)
{
    if(*err == LIBCIRCLE_MPI_ERROR) {
        LOG(CIRCLE_LOG_ERR, "Libcircle received abort signal, checkpointing.");
    }
    else {
        LOG(CIRCLE_LOG_ERR, "Libcircle received MPI error, checkpointing.");
    }

    CIRCLE_checkpoint();

    return;
}
#pragma GCC diagnostic warning "-Wunused-parameter"

/**
 * Wrapper for pushing an element on the queue
 *
 */
int8_t CIRCLE_enqueue(char* element)
{
    return CIRCLE_internal_queue_push(CIRCLE_INPUT_ST.queue, element);
}

/**
 * Wrapper for popping an element
 */
int8_t CIRCLE_dequeue(char* element)
{
    return CIRCLE_internal_queue_pop(CIRCLE_INPUT_ST.queue, element);
}

/**
 * Wrapper for getting the local queue size
 */
uint32_t CIRCLE_local_queue_size()
{
    return CIRCLE_INPUT_ST.queue->count;
}

/**
 * Wrapper for reading in restart files
 */
int8_t _CIRCLE_read_restarts()
{
    return CIRCLE_internal_queue_read(CIRCLE_INPUT_ST.queue, \
                                      CIRCLE_global_rank);
}

/**
 * Wrapper for checkpointing
 */
int8_t _CIRCLE_checkpoint()
{
    return CIRCLE_internal_queue_write(CIRCLE_INPUT_ST.queue, \
                                       CIRCLE_global_rank);
}

/**
 * Initializes all variables local to a rank
 */
void CIRCLE_init_local_state(CIRCLE_state_st* local_state, int32_t size)
{
    int i = 0;
    local_state->token = WHITE;
    local_state->have_token = 0;
    local_state->term_flag = 0;
    local_state->work_flag = 0;
    local_state->work_pending_request = 0;
    local_state->request_pending_receive = 0;
    local_state->term_pending_receive = 0;
    local_state->incoming_token = BLACK;

    local_state->request_offsets = (uint32_t*) calloc(\
                                   CIRCLE_INITIAL_INTERNAL_QUEUE_SIZE, \
                                   sizeof(uint32_t));
    local_state->work_offsets = (uint32_t*) calloc(\
                                CIRCLE_INITIAL_INTERNAL_QUEUE_SIZE, \
                                sizeof(unsigned int));
    local_state->request_flag = (int32_t*) calloc(size, sizeof(int32_t));
    local_state->request_recv_buf = (int32_t*) calloc(size, sizeof(int32_t));

    local_state->mpi_state_st->request_status = \
            (MPI_Status*) malloc(sizeof(MPI_Status) * size);
    local_state->mpi_state_st->request_request = \
            (MPI_Request*) malloc(sizeof(MPI_Request) * size);

    local_state->mpi_state_st->work_comm = CIRCLE_INPUT_ST.work_comm;
    local_state->mpi_state_st->token_comm = CIRCLE_INPUT_ST.token_comm;

    for(i = 0; i < size; i++) {
        local_state->mpi_state_st->request_request[i] = MPI_REQUEST_NULL;
    }

    local_state->work_request_tries = 0;
    return;
}

/**
 * @brief Function that actually does work, calls user callback.
 *
 * This is the main body of execution.
 *
 * - For every work loop execution, the following happens:
 *     -# Check for work requests from other ranks.
 *     -# If this rank has work, call the user callback function.
 *     -# If this rank doesn't have work, ask a random rank for work.
 *     -# If after requesting work, this rank still doesn't have any,
 *        check for termination conditions.
 */
void CIRCLE_work_loop(CIRCLE_state_st* sptr, CIRCLE_handle* queue_handle)
{
    int token = WHITE;
    int work_status = -1;
    int term_status = -1;

    /* Loop until done */
    while(token != DONE) {
        /* Check for and service work requests */
        //LOG(CIRCLE_LOG_DBG, "Checking for requestlocal_state...");
        CIRCLE_check_for_requests(CIRCLE_INPUT_ST.queue, sptr);

        /* If I have no work, request work from another rank */
        if(CIRCLE_INPUT_ST.queue->count == 0) {
            work_status = CIRCLE_request_work(CIRCLE_INPUT_ST.queue, sptr);

            if(work_status == TERMINATE) {
                token = DONE;
                LOG(CIRCLE_LOG_DBG, "Received termination signal.");
            }
        }

        /* If I have some work, process one work item */
        if(CIRCLE_INPUT_ST.queue->count > 0 && !CIRCLE_ABORT_FLAG) {
            (*(CIRCLE_INPUT_ST.process_cb))(queue_handle);
            local_objects_processed++;
        }
        /* If I don't have work, check for termination */
        else if(token != DONE) {
            term_status = CIRCLE_check_for_term(sptr);

            if(term_status == TERMINATE) {
                token = DONE;
                LOG(CIRCLE_LOG_DBG, "Received termination signal.");
            }
        }
    }

    return;
}

/**
 * @brief Responds with no_work to pending work requests.
 *
 * Answers any pending work requests in case a rank is blocking,
 * waiting for a response.
 */
void CIRCLE_cleanup_mpi_messages(CIRCLE_state_st* sptr)
{
    uint32_t i = 0;
    uint32_t j = 0;

    /* Make sure that all pending work requests are answered. */
    for(j = 0; j < sptr->size; j++) {
        for(i = 0; i < sptr->size; i++) {
            if(i != sptr->rank) {
                sptr->request_flag[i] = 0;

                if(MPI_Test(&sptr->mpi_state_st->request_request[i], \
                            &sptr->request_flag[i], \
                            &sptr->mpi_state_st->request_status[i]) \
                        != MPI_SUCCESS) {

                    MPI_Abort(*sptr->mpi_state_st->work_comm, \
                              LIBCIRCLE_MPI_ERROR);
                }

                if(sptr->request_flag[i]) {
                    CIRCLE_send_no_work(i);
                    MPI_Start(&sptr->mpi_state_st->request_request[i]);
                }
            }
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
    int rank = -1;
    int size = -1;
    int i = -1;

    /* Holds all worker state */
    CIRCLE_state_st local_state;
    CIRCLE_state_st* sptr = &local_state;

    /* Holds all mpi state */
    CIRCLE_mpi_state_st mpi_s;
    local_state.mpi_state_st = &mpi_s;

    /* Provides an interface to the queue. */
    CIRCLE_handle queue_handle;
    queue_handle.enqueue = &CIRCLE_enqueue;
    queue_handle.dequeue = &CIRCLE_dequeue;
    queue_handle.local_queue_size = &CIRCLE_local_queue_size;

    MPI_Comm_size(*CIRCLE_INPUT_ST.work_comm, &size);
    CIRCLE_init_local_state(sptr, size);
    MPI_Errhandler circle_err;
    MPI_Comm_create_errhandler(CIRCLE_MPI_error_handler, &circle_err);
    MPI_Comm_set_errhandler(*mpi_s.work_comm, circle_err);

    rank = CIRCLE_global_rank;
    srand(rank);
    local_state.rank = rank;
    local_state.size = size;
    local_state.next_processor = (rank + 1) % size;
    local_state.token_partner = (rank - 1) % size;

    if(local_state.token_partner < 0) {
        local_state.token_partner = size - 1;
    }

    /* Initial local state */
    local_objects_processed = 0;
    total_objects_processed = 0;

    /* Master rank starts out with the initial data creation */
    uint32_t* total_objects_processed_array = (uint32_t*) calloc(size, sizeof(uint32_t));

    if(rank == 0) {
        (*(CIRCLE_INPUT_ST.create_cb))(&queue_handle);
        local_state.have_token = 1;
    }

    CIRCLE_work_loop(sptr, &queue_handle);
    CIRCLE_cleanup_mpi_messages(sptr);

    if(CIRCLE_ABORT_FLAG) {
        CIRCLE_checkpoint();
    }

    MPI_Gather(&local_objects_processed, 1, MPI_INT, \
               &total_objects_processed_array[0], 1, MPI_INT, 0, \
               *mpi_s.work_comm);
    MPI_Reduce(&local_objects_processed, &total_objects_processed, 1, \
               MPI_INT, MPI_SUM, 0, *mpi_s.work_comm);

    if(rank == 0) {
        for(i = 0; i < size; i++) {
            LOG(CIRCLE_LOG_INFO, "Rank %d\tObjects Processed %d\t%lf%%\n", i, \
                total_objects_processed_array[i], \
                (double)total_objects_processed_array[i] / \
                (double)total_objects_processed * 100.0);
        }
    }

    if(rank == 0) {
        LOG(CIRCLE_LOG_INFO, \
            "Total Objects Processed: %d\n", total_objects_processed);
    }

    return 0;
}

/* EOF */

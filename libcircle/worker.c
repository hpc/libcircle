/*! \file worker.c
 *  \authors Jon Bringhurst, Jharrod LaFon
 */
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <mpi.h>

#include "log.h"
#include "libcircle.h"
#include "token.h"
#include "lib.h"
#include "worker.h"

extern CIRCLE_input_st CIRCLE_INPUT_ST;

int CIRCLE_ABORT_FLAG = 0;

/* 
 * \brief Function to be called in the event of an MPI error.
 * This function get registered with MPI to be called
 * in the event of an MPI Error.  It attempts
 * to checkpoint.
 */
void
CIRCLE_MPI_error_handler(MPI_Comm *comm, int *err, ...)
{
    if(*err == LIBCIRCLE_MPI_ERROR)
        LOG(CIRCLE_LOG_ERR,"Libcircle received abort signal, checkpointing.");
    else
        LOG(CIRCLE_LOG_ERR,"Libcircle received MPI error, checkpointing.");
    CIRCLE_checkpoint();
    return;
}

/* \brief Wrapper for pushing an element on the queue */
int
CIRCLE_enqueue(char *element)
{
    return CIRCLE_queue_push(CIRCLE_INPUT_ST.queue, element);
}

/* \brief Wrapper for popping an element */
int
CIRCLE_dequeue(char *element)
{
    return CIRCLE_queue_pop(CIRCLE_INPUT_ST.queue, element);
}

/* \brief Wrapper for getting the local queue size */
int
CIRCLE_local_queue_size()
{
    return CIRCLE_INPUT_ST.queue->count;
}

/* \brief Wrapper for reading in restart files */
int
_CIRCLE_read_restarts()
{
    return CIRCLE_queue_read(CIRCLE_INPUT_ST.queue, CIRCLE_global_rank);
}

/* \brief Wrapper for checkpointing */
int
_CIRCLE_checkpoint()
{
    return CIRCLE_queue_write(CIRCLE_INPUT_ST.queue, CIRCLE_global_rank);
}

/* \brief Initializes all variables local to a rank */
void 
CIRCLE_init_local_state(CIRCLE_state_st * local_state,int size)
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
    local_state->request_offsets = (unsigned int*) calloc(CIRCLE_INITIAL_QUEUE_SIZE,sizeof(unsigned int));
    local_state->work_offsets = (unsigned int*) calloc(CIRCLE_INITIAL_QUEUE_SIZE,sizeof(unsigned int));
    local_state->mpi_state_st->request_status = (MPI_Status *) malloc(sizeof(MPI_Status)*size);
    local_state->request_flag = (int *) calloc(size,sizeof(int));
    local_state->request_recv_buf = (int *) calloc(size,sizeof(int));
    local_state->mpi_state_st->request_request = (MPI_Request*) malloc(sizeof(MPI_Request)*size);
    for(i = 0; i < size; i++)
        local_state->mpi_state_st->request_request[i] = MPI_REQUEST_NULL;
    local_state->work_request_tries = 0;
    return;
}

/* 
 * \brief Function that actually does work, calls user callback.
 * This is the main body of execution.  
 * For every work loop execution, the following happens:
 * 1) Check for work requests from other ranks.
 * 2) If this rank has work, call the user callback function.
 *    If this rank doesn't have work, ask a random rank for work.
 * 3) If after requesting work, this rank still doesn't have any,
 *    check for termination conditions.
 */
void
CIRCLE_work_loop(CIRCLE_state_st * sptr,CIRCLE_handle * queue_handle)
{
    int token = WHITE;
    int work_status = -1;
    int term_status = -1;
    /* Loop until done */
    while(token != DONE)
    {
        /* Check for and service work requests */
        //LOG(CIRCLE_LOG_DBG, "Checking for requestlocal_state...");
        CIRCLE_check_for_requests(CIRCLE_INPUT_ST.queue,sptr);

        /* If I have no work, request work from another rank */
        if(CIRCLE_INPUT_ST.queue->count == 0)
        {
            work_status = CIRCLE_request_work(CIRCLE_INPUT_ST.queue,sptr);
            if(work_status == TERMINATE)
            {
                token = DONE;
                LOG(CIRCLE_LOG_DBG,"Received termination signal.");
            }
        }
        /* If I have some work, process one work item */
        if(CIRCLE_INPUT_ST.queue->count > 0 && !CIRCLE_ABORT_FLAG)
        {
            (*(CIRCLE_INPUT_ST.process_cb))(queue_handle);
        }
        /* If I don't have work, check for termination */
        else if(token != DONE)
        {
            term_status = CIRCLE_check_for_term(sptr);
            if(term_status == TERMINATE)
            {
                token = DONE;
                LOG(CIRCLE_LOG_DBG,"Received termination signal.");
            }
        }
    }
    return;
}

/* 
 * \brief Responds with no_work to pending work requests.
 * Answers any pending work requests in case a rank is blocking,
 * waiting for a response.
 */
void 
CIRCLE_cleanup_mpi_messages(CIRCLE_state_st * sptr)
{
    int i = 0;
    int j = 0;
    /* Make sure that all pending work requests are answered. */
    for(j = 0; j < sptr->size; j++)
     for(i = 0; i < sptr->size; i++)
        if(i != sptr->rank)
        {
            sptr->request_flag[i] = 0;
            if(MPI_Test(&sptr->mpi_state_st->request_request[i], \
                    &sptr->request_flag[i], &sptr->mpi_state_st->request_status[i]) \
                    != MPI_SUCCESS)
                MPI_Abort(MPI_COMM_WORLD,LIBCIRCLE_MPI_ERROR);
            if(sptr->request_flag[i])
            {
                CIRCLE_send_no_work(i);
                MPI_Start(&sptr->mpi_state_st->request_request[i]);
            }
        }
    return;
}

/*
 * \brief Sets up libcircle, calls work loop function
 * Main worker function.  This function:
 * Initializes MPI
 * Initializes internal libcircle data structures
 * Calls libcircle's main work loop function.
 * Checkpoints if CIRCLE_abort has been called by a rank. 
 */
int
CIRCLE_worker()
{
    int rank = -1;
    int size = -1;
    int next_processor = -1;
    int token_partner = -1;
    /* Holds all worker state */
    CIRCLE_state_st local_state;
    CIRCLE_state_st * sptr = &local_state;

    /* Holds all mpi state */
    CIRCLE_mpi_state_st mpi_s;
    local_state.mpi_state_st = &mpi_s;

    /* Holds work elements */

    /* Provides an interface to the queue. */
    CIRCLE_handle queue_handle;
    queue_handle.enqueue = &CIRCLE_enqueue;
    queue_handle.dequeue = &CIRCLE_dequeue;
    queue_handle.local_queue_size = &CIRCLE_local_queue_size;
  
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Errhandler circle_err;
    MPI_Comm_create_errhandler(CIRCLE_MPI_error_handler,&circle_err);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD,circle_err);
    
    srand(rank);
    rank = CIRCLE_global_rank;
    local_state.rank = rank;
    local_state.size = size;
    next_processor = (rank+1) % size;
    token_partner = (rank-1) % size;
    local_state.next_processor = (rank+1) % size;
    local_state.token_partner = (rank-1) % size;
    if(token_partner < 0) token_partner = size-1;
     /* Initial local state */
    CIRCLE_init_local_state(sptr,size);
    /* Master rank starts out with the initial data creation */
    if(rank == 0)
    {
        (*(CIRCLE_INPUT_ST.create_cb))(&queue_handle);
        local_state.have_token = 1;
    }
    CIRCLE_work_loop(sptr,&queue_handle);
    CIRCLE_cleanup_mpi_messages(sptr);
    if(CIRCLE_ABORT_FLAG)
        CIRCLE_checkpoint();
    MPI_Barrier(MPI_COMM_WORLD);
    return 0;
}

/* EOF */

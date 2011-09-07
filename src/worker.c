#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <mpi.h>

#include "log.h"
#include "libcircle.h"
#include "token.h"
#include "lib.h"

extern CIRCLE_input_st CIRCLE_INPUT_ST;

int
CIRCLE_enqueue(char *element)
{
    return CIRCLE_queue_push(CIRCLE_INPUT_ST.queue, element);
}

int
CIRCLE_dequeue(char *element)
{
    return CIRCLE_queue_pop(CIRCLE_INPUT_ST.queue, element);
}

int
CIRCLE_worker()
{
    double start_time;
    int token = WHITE;
    int token_partner;
    
    /* Holds all worker state */
    CIRCLE_state_st s;
    CIRCLE_state_st * sptr = &s;

    /* Holds all mpi state */
    CIRCLE_mpi_state_st mpi_s;
    s.mpi_state_st = &mpi_s;

    /* Holds work elements */
    CIRCLE_queue_t queue;
    CIRCLE_INPUT_ST.queue = &queue;

    /* Provides an interface to the queue. */
    CIRCLE_handle queue_handle;
    queue_handle.enqueue = &CIRCLE_enqueue;
    queue_handle.dequeue = &CIRCLE_dequeue;

    /* Memory for work queue */
    queue.base = (char*) malloc(sizeof(char) * CIRCLE_MAX_STRING_LEN * CIRCLE_INITIAL_QUEUE_SIZE);
    
    /* A pointer to each string in the queue */
    queue.strings = (char **) malloc(sizeof(char*) * CIRCLE_INITIAL_QUEUE_SIZE);
    
    CIRCLE_queue_t * qp = &queue;
    
    queue.head = queue.base;
    queue.end = queue.base + (CIRCLE_MAX_STRING_LEN * CIRCLE_INITIAL_QUEUE_SIZE);
    queue.count = 0;
    int rank = -1;
    int size = -1;
    int next_processor;
    //int cycles = 0;
    
    /* Get MPI info */
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    srand(rank);
    s.rank = rank;
    s.size = size;
    s.token = WHITE;
    next_processor = (rank+1) % size;
    token_partner = (rank-1)%size;
    s.next_processor = (rank+1) % size;
    s.token_partner = (rank-1)%size;
    if(token_partner < 0) token_partner = size-1;
   
    /* Initial state */
    s.have_token = 0;
    s.term_flag = 0;
    s.work_flag = 0;
    s.work_pending_request = 0;
    s.request_pending_receive = 0;
    s.term_pending_receive = 0;
    qp->num_stats = 0;
    s.incoming_token = BLACK;
    s.request_offsets = (unsigned int*) calloc(CIRCLE_INITIAL_QUEUE_SIZE,sizeof(unsigned int));
    s.work_offsets = (unsigned int*) calloc(CIRCLE_INITIAL_QUEUE_SIZE,sizeof(unsigned int));
    s.mpi_state_st->request_status = (MPI_Status *) malloc(sizeof(MPI_Status)*size);
    int i = 0;
    s.request_flag = (int *) calloc(size,sizeof(int));
    s.request_recv_buf = (int *) calloc(size,sizeof(int));
    s.mpi_state_st->request_request = (MPI_Request*) malloc(sizeof(MPI_Request)*size);
    for(i = 0; i < size; i++)
        s.mpi_state_st->request_request[i] = MPI_REQUEST_NULL;
    s.work_request_tries = 0;
    
    /* Master rank starts out with the initial data creation */
    if(rank == 0)
    {
        (*(CIRCLE_INPUT_ST.create_cb))(&queue_handle);
        s.have_token = 1;
    }
    start_time = MPI_Wtime();
    /* Loop until done */
    while(token != DONE)
    {
        /* Check for and service work requests */
        LOG(LOG_DBG, "Checking for requests...");
        CIRCLE_check_for_requests(qp,sptr);
        LOG(LOG_DBG, "done");

        if(qp->count == 0)
        {
            LOG(LOG_DBG, "Requesting work...");

            //cleanup_work_messages(sptr);
            if(CIRCLE_request_work(qp,sptr) < 0)
                token = DONE;

            LOG(LOG_DBG, "Done requesting work");
        }

        if(qp->count > 0)
        {
            (*(CIRCLE_INPUT_ST.process_cb))(&queue_handle);
        }
        else if(token != DONE)
        {
            LOG(LOG_DBG, "Checking for termination...");
            if(CIRCLE_check_for_term(sptr) == TERMINATE)
                token = DONE;
            LOG(LOG_DBG, "done");
        }
    }

    int j = 0;

    for(j = 0; j < sptr->size; j++)
     for(i = 0; i < sptr->size; i++)
        if(i != sptr->rank)
        {
            sptr->request_flag[i] = 0;
            if(MPI_Test(&sptr->mpi_state_st->request_request[i], \
                    &sptr->request_flag[i], &sptr->mpi_state_st->request_status[i]) \
                    != MPI_SUCCESS)
                exit(1);
            if(sptr->request_flag[i])
            {
                CIRCLE_send_no_work(i,sptr);
                MPI_Start(&sptr->mpi_state_st->request_request[i]);
            }
        }

    LOG(LOG_DBG, "[Rank %d] Stats: %d", rank, qp->num_stats);

    return 0;
}

/* EOF */

/*! \file pstat.c
 *  \authors Jharrod LaFon, Jon Bringhurst
 */

#include <mpi.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "libcircle.h"
#include "log.h"
#include "token.h"
#include "queue.h"

extern int CIRCLE_ABORT_FLAG;

void CIRCLE_bcast_abort(void)
{
    LOG(CIRCLE_LOG_WARN,"Libcircle abort started from %d",CIRCLE_global_rank);
    int buffer = ABORT;
    int size = 0;
    int i = 0;
    int rank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    CIRCLE_ABORT_FLAG = 1;
    for(i = 0; i < size; i++)
    {
        if(i != rank)
        {
            MPI_Send(&buffer, 1, MPI_INT, i, \
                WORK_REQUEST, MPI_COMM_WORLD);
            LOG(CIRCLE_LOG_WARN,"Libcircle abort message sent to %d",i);
        }
    }

return; 
}
/*! \brief Checks for incoming tokens, determines termination conditions.
 * 
 * When the master rank is idle, it generates a token that is initially white.
 * When a node is idle, and can't get work for one loop iteration then it
 * checks for termination.  It checks to see if the token has been passed to it,
 * additionally checking for the termination token.  If a rank receives a black
 * token then it forwards a black token. Otherwise it forwards its own color.
 *
 * All nodes start out in the white state.  State is *not* the same thing as
 * the token. If a node j sends work to a rank i (i < j) then its state turns
 * black. It then turns the token black when it comes around, forwards it, and
 * turns its state back to white.
 */
int
CIRCLE_check_for_term(CIRCLE_state_st *st)
{
    /* If I have the token (I am already idle) */
    if(st->have_token)
    {
        /* The master rank generates the original WHITE token */
        if(st->rank == 0)
        {
         //   LOG(CIRCLE_LOG_DBG, "Master generating WHITE token.");

            st->incoming_token = WHITE;
            MPI_Send(&st->incoming_token, 1, MPI_INT, (st->rank+1) % st->size, \
                     TOKEN, MPI_COMM_WORLD);
            st->token = WHITE;
            st->have_token = 0;
        
            /*
             * Immediately post a receive to listen for the token when it
             * comes back around
             */
            MPI_Irecv(&st->incoming_token, 1, MPI_INT, st->token_partner, \
                      TOKEN, MPI_COMM_WORLD, &st->mpi_state_st->term_request);

            st->term_pending_receive = 1;
        }
        else
        {
            /*
             * In this case I am not the master rank.
             *
             * Turn it black if I am in a black state, and forward it since
             * I am idle.
             *
             * Then I turn my state white.
             */ 
            if(st->token == BLACK)
                st->incoming_token = BLACK;

            MPI_Send(&st->incoming_token, 1, MPI_INT, (st->rank+1)%st->size, \
                     TOKEN, MPI_COMM_WORLD);

            st->token = WHITE;
            st->have_token = 0;

            /*
             * Immediately post a receive to listen for the token when it
             * comes back around.
             */
            MPI_Irecv(&st->incoming_token, 1, MPI_INT, st->token_partner, \
                      TOKEN, MPI_COMM_WORLD, &st->mpi_state_st->term_request);

            st->term_pending_receive = 1;
        }

        return 0;
    }
    /* If I don't have the token. */
    else
    {
        /* Check to see if I have posted a receive. */
        if(!st->term_pending_receive)
        {
            st->incoming_token = -1;
            MPI_Irecv(&st->incoming_token, 1, MPI_INT, st->token_partner, \
                      TOKEN, MPI_COMM_WORLD, &st->mpi_state_st->term_request);
            st->term_pending_receive = 1;
        }

        st->term_flag = 0;

        /* Check to see if my pending receive has completed */
        MPI_Test(&st->mpi_state_st->term_request, &st->term_flag, \
            &st->mpi_state_st->term_status);

        if(!st->term_flag)
        {
            return 0;
        }
        
        /* If I get here, then I received the token */
        st->term_pending_receive = 0;
        st->have_token = 1;
       // LOG(CIRCLE_LOG_DBG,"Received token %d\n",st->incoming_token);
        /* Check for termination */
        if(st->incoming_token == TERMINATE)
        {
          //  LOG(CIRCLE_LOG_DBG, "Received termination token");
            st->token = TERMINATE;
            MPI_Send(&st->token, 1, MPI_INT, (st->rank+1) % st->size, \
                     TOKEN,MPI_COMM_WORLD);

            //LOG(CIRCLE_LOG_DBG, "Forwared termination token");

            return TERMINATE;
        }
        if(st->token == BLACK && st->incoming_token == BLACK)
        {
            st->token = WHITE;
        }

        if(st->rank == 0 && st->incoming_token == WHITE)
        {
            LOG(CIRCLE_LOG_DBG, "Master has detected termination.");

            st->token = TERMINATE;
            MPI_Send(&st->token, 1, MPI_INT, 1, TOKEN, MPI_COMM_WORLD);
            MPI_Send(&st->token, 1, MPI_INT, 1, WORK, MPI_COMM_WORLD);

            return TERMINATE;
        }
    }

    return 0;
}

/*! \brief Generates a random rank
 *
 * This returns a random rank (not yourself)
 */
inline int
CIRCLE_get_next_proc(int rank, int size)
{
    int result = rand() % size;

    while(result == rank)
    {
        result = rand() % size;
    }

    return result;
}

/*! \brief Waits on an incoming message.
 * 
 * This function will spin lock waiting on a message to arrive from the
 * given source, with the given tag.  It does *not* receive the message.
 * If a message is pending, it will return it's size.  Otherwise
 * it returns 0.
 */
int
CIRCLE_wait_on_probe(CIRCLE_state_st *st, int source, int tag)
{
    int flag = 0;
    int i = 0;
    MPI_Status temp;
    while(!flag)
    {
        MPI_Iprobe(source, tag, MPI_COMM_WORLD, &flag,&temp);
        for(i = 0; i < st->size; i++)
        {
            st->request_flag[i] = 0;
            if(i != st->rank)
            {
                MPI_Test(&st->mpi_state_st->request_request[i], \
                    &st->request_flag[i], &st->mpi_state_st->request_status[i]);
                {
                    if(st->request_flag[i])
                    {
                        CIRCLE_send_no_work(i);
                        MPI_Start(&st->mpi_state_st->request_request[i]);
                    }

                }
            }
        }

        if(CIRCLE_check_for_term(st) == TERMINATE)
            return TERMINATE; 
    }
    if(flag)
    {
        return temp._count;
    }
    else
    {
        return 0;
    }
}

/*! \brief Requests work from other ranks 
 * 
 * Somewhat complicated, but essentially it requests work from a random 
 * rank and gives up after a short timeout.  If it is successful in getting
 * work, the work is received directly into the work queue.
 */
int
CIRCLE_request_work(CIRCLE_queue_t *qp, CIRCLE_state_st *st)
{
    int temp_buffer = 3;
    if(CIRCLE_ABORT_FLAG)
        temp_buffer = ABORT;

    //LOG(CIRCLE_LOG_DBG, "Sending work request to %d...",st->next_processor);

    /* Send work request. */
    MPI_Send(&temp_buffer, 1, MPI_INT, st->next_processor, \
        WORK_REQUEST, MPI_COMM_WORLD);

    //LOG(CIRCLE_LOG_DBG, "done.");

    //LOG(CIRCLE_LOG_DBG, "Getting response from %d...", st->next_processor);
    st->work_offsets[0] = 0;

    /* Wait for an answer... */
    int size = CIRCLE_wait_on_probe(st, st->next_processor, WORK);
    
    if(size == TERMINATE)
    {
        return TERMINATE;
    }

    if(size == 0)
    {
        //LOG(CIRCLE_LOG_DBG, "No response from %d", st->next_processor);
        st->next_processor = CIRCLE_get_next_proc(st->rank, st->size);

        return 0;
    }

    //LOG(CIRCLE_LOG_DBG, "Received message with %d characters.", size);

    /* If we get here, there was definitely an answer.  Receives the offsets then */
    MPI_Recv(st->work_offsets, size, MPI_INT, st->next_processor, \
        WORK, MPI_COMM_WORLD, &st->mpi_state_st->work_offsets_status);

    /* We'll ask somebody else next time */
    int source = st->next_processor;

    st->next_processor = CIRCLE_get_next_proc(st->rank, st->size);

    int chars = st->work_offsets[1];
    int items = st->work_offsets[0];

    if(items == -1)
    {
        return -1;
    }
    else if(items == 0)
    {
        //LOG(CIRCLE_LOG_DBG, "Received no work.");
        return 0;
    }
    else if(items == ABORT)
    {
        CIRCLE_ABORT_FLAG = 1;
        //LOG(CIRCLE_LOG_DBG, "I am now POISONED");
        return ABORT;
    }

    //LOG(CIRCLE_LOG_DBG, "Getting work from %d, %d items.", source, items);
    
    /* Wait and see if they sent the work over */
    size = CIRCLE_wait_on_probe(st, source, WORK);

    if(size == TERMINATE)
    {
        return TERMINATE;
    }

    if(size == 0)
    {
        return 0;
    }

    //LOG(CIRCLE_LOG_DBG, "Message pending with %d size", size);

    /*
     * Good, we have a pending message from source with a WORK tag.
     * It can only be a work queue.
     */
    MPI_Recv(qp->base, (chars + 1) * sizeof(char), MPI_BYTE, source, WORK, \
        MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    qp->count = items;
    int i = 0;
    for(i = 0; i < qp->count; i++)
    {
        qp->strings[i] = qp->base + st->work_offsets[i+2];
        //LOG(CIRCLE_LOG_DBG, "Item [%d] Offset [%d] String [%s]",i, st->work_offsets[i + 2], qp->strings[i]);
    }

    if(size == 0)
    {
        qp->count = 0;
        return 0;
    }

    if(qp->strings[0] != qp->base) {
        LOG(CIRCLE_LOG_FATAL, "The base address of the queue doesn't match what it should be.");
        exit(EXIT_FAILURE);
    }

    qp->head = qp->strings[qp->count - 1] + strlen(qp->strings[qp->count - 1]);

    //LOG(CIRCLE_LOG_DBG, "Received items. Queue size now %d", qp->count);
    
    CIRCLE_queue_print(qp);
    return 0;
}
/*! \brief Sends a no work reply to someone requesting work. */
void
CIRCLE_send_no_work(int dest)
{
    int no_work[2];

    no_work[0] = (CIRCLE_ABORT_FLAG)?ABORT:0;
    no_work[1] = 0;

    //if(CIRCLE_ABORT_FLAG)
      //  LOG(CIRCLE_LOG_DBG, "Received work request from %d, but am POISONED!", dest);
   // else
     //   LOG(CIRCLE_LOG_DBG, "Received work request from %d, but have no work.", dest);

    MPI_Request r;
    MPI_Isend(&no_work, 1, MPI_INT, dest, WORK, MPI_COMM_WORLD, &r);
    MPI_Wait(&r, MPI_STATUS_IGNORE);

    //LOG(CIRCLE_LOG_DBG, "Response sent to %d, have no work.", dest);
}

/*! \brief Distributes a random amount of the local work queue to the n requestors */
void
CIRCLE_send_work_to_many(CIRCLE_queue_t *qp, CIRCLE_state_st *st,\
                              int *requestors, int rcount)
{
    int i = 0;
    if(rcount <= 0)
    {
        LOG(CIRCLE_LOG_FATAL, "Something is wrong with the amount of work we think we have.");
        exit(EXIT_FAILURE);
    }

    /* Random number between rcount+1 and qp->count */
    int total_amount = rand() % (qp->count)+1;

    //LOG(CIRCLE_LOG_DBG, "Queue size: %d, Total_amount: %d", qp->count, total_amount);
    /* Get size of chunk */
    int increment = total_amount / rcount;

    for(i = 0; i < rcount; i ++)
    {
        total_amount -= increment;
        if(total_amount < increment)
        {
            increment += total_amount;
        }
        CIRCLE_send_work( qp, st, requestors[i], increment);
    }
}

/* \brief Sends work to a requestor */
int
CIRCLE_send_work(CIRCLE_queue_t *qp, CIRCLE_state_st *st,\
                     int dest, int count )
{
    if(count <= 0)
    {
        return 0;
    }
    /* For termination detection */
    if(dest < st->rank || dest == st->token_partner)
    {
        st->token = BLACK;
    }

    /* Base address of the buffer to be sent */
    char *b = qp->strings[qp->count - count];

    /* Address of the beginning of the last string to be sent */
    char *e = qp->strings[qp->count - 1];

    /* Distance between them */
    size_t diff = e - b;
    diff += strlen(e);

    if(qp->count < 10)
    {
        CIRCLE_queue_print(qp);
    }

    /* offsets[0] = number of strings */
    /* offsets[1] = number of chars being sent */
    st->request_offsets[0] = count;
    st->request_offsets[1] = diff;

    if(diff >= (CIRCLE_INITIAL_QUEUE_SIZE * CIRCLE_MAX_STRING_LEN)) {
        LOG(CIRCLE_LOG_FATAL, "We're trying to throw away part of the queue for some reason.");
        exit(EXIT_FAILURE);
    }

    int j = qp->count - count;
    int i = 0;

    for(i=0; i < (int)st->request_offsets[0]; i++)
    {
        st->request_offsets[i + 2] = qp->strings[j++] - b;

        //LOG(CIRCLE_LOG_DBG, "[j=%d] Base address: %p, String[%d] address: %p, String \"%s\" Offset: %u", j, b, i, qp->strings[j - 1], qp->strings[j - 1], st->request_offsets[i + 2]);
    }

    /* offsets[qp->count - qp->count/2+2]  is the size of the last string */
    st->request_offsets[count + 2] = strlen(qp->strings[qp->count - 1]);

    //LOG(CIRCLE_LOG_DBG, "\tSending offsets for %d items to %d...",st->request_offsets[0], dest);

    MPI_Ssend(st->request_offsets, st->request_offsets[0]+2, \
        MPI_INT, dest, WORK, MPI_COMM_WORLD);

    //LOG(CIRCLE_LOG_DBG, "\tSending buffer to %d...", dest);

    MPI_Ssend(b, (diff + 1) * sizeof(char), MPI_BYTE, dest, WORK, MPI_COMM_WORLD);
    qp->count = qp->count - count;

    //LOG(CIRCLE_LOG_DBG, "sent %d items to %d.", st->request_offsets[0], dest);

    return 0;
}

/*! \brief Checks for outstanding work requests */
int
CIRCLE_check_for_requests(CIRCLE_queue_t *qp, CIRCLE_state_st *st)
{
    int *requestors = (int *)calloc(st->size, sizeof(int));
    int i = 0;
    int rcount = 0;

    /* This loop is only excuted once.  It is used to initiate receives. 
     * When a received is completed, we repost it immediately to capture
     * the next request */
    if(!st->request_pending_receive)
    {
        for(i = 0; i < st->size; i++)
        {
            if(i != st->rank)
            {
                st->request_recv_buf[i] = 0;
                MPI_Recv_init(&st->request_recv_buf[i], 1, MPI_INT, i, \
                    WORK_REQUEST, MPI_COMM_WORLD, \
                    &st->mpi_state_st->request_request[i]);

                MPI_Start(&st->mpi_state_st->request_request[i]);
            }
        }
        st->request_pending_receive = 1;
    }

    /* Test to see if any posted receive has completed */
    for(i = 0; i < st->size; i++)
    {
        if(i != st->rank)
        {
            st->request_flag[i] = 0;
            if(MPI_Test(&st->mpi_state_st->request_request[i], \
                    &st->request_flag[i], \
                    &st->mpi_state_st->request_status[i]) != MPI_SUCCESS)
                exit(EXIT_FAILURE);

            if(st->request_flag[i])
            {
                if(st->request_recv_buf[i] == ABORT)
                {
                    CIRCLE_ABORT_FLAG = 1;
                    //LOG(CIRCLE_LOG_DBG,"I am now POISONED.");
                    return ABORT;
                }
                requestors[rcount++] = i;
                st->request_flag[i] = 0;
            }
        }
    }

    /* If we didn't received any work request, no need to continue */
    if(rcount == 0)
    {
        return 0;
    }

    if(qp->count == 0 || CIRCLE_ABORT_FLAG)
    {
        for(i = 0; i < rcount; i++)
        {
            CIRCLE_send_no_work(requestors[i]);
        }
    }
    /* If you get here, then you have work to send AND the CIRCLE_ABORT_FLAG is not set */
    else
    {
        //LOG(CIRCLE_LOG_DBG, "Got work requests from %d ranks.",rcount);
        CIRCLE_send_work_to_many( qp, st, requestors, rcount);
    }

    for(i = 0; i < rcount; i++)
    {
        MPI_Start(&st->mpi_state_st->request_request[requestors[i]]);
    }

    free(requestors);

    return 0;
}

void
CIRCLE_print_offsets(unsigned int *offsets, int count)
{
    int i = 0;

    for(i = 0; i < count; i++)
    {
        LOG(CIRCLE_LOG_DBG, "\t[%d] %d", i, offsets[i]);
    }
}

/* EOF */

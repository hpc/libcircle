/**
 * @file
 *
 * Handles features of libcircle related to tokens (for self stabilization).
 */

#include <mpi.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>

#include "libcircle.h"
#include "log.h"
#include "token.h"
#include "queue.h"

extern int8_t CIRCLE_ABORT_FLAG;
extern CIRCLE_input_st CIRCLE_INPUT_ST;
/**
 * Sends an abort message to all ranks.
 *
 * This function is used to send a 'poisoned' work request to each rank, so
 * that they will know to abort.
 */
void CIRCLE_bcast_abort(void)
{
    LOG(CIRCLE_LOG_WARN, \
        "Libcircle abort started from %d", CIRCLE_global_rank);

    int buffer = ABORT;
    int size = 0;
    int i = 0;
    int32_t rank = -1;

    MPI_Comm_rank(*CIRCLE_INPUT_ST.work_comm, &rank);
    MPI_Comm_size(*CIRCLE_INPUT_ST.work_comm, &size);

    CIRCLE_ABORT_FLAG = 1;

    for(i = 0; i < size; i++) {
        if(i != rank) {
            MPI_Send(&buffer, 1, MPI_INT, i, \
                     WORK_REQUEST, *CIRCLE_INPUT_ST.work_comm);
            LOG(CIRCLE_LOG_WARN, "Libcircle abort message sent to %d", i);
        }
    }

    return;
}

/**
 * Checks for incoming tokens, determines termination conditions.
 *
 * When the master rank is idle, it generates a token that is initially white.
 * When a node is idle, and can't get work for one loop iteration, then it
 * checks for termination. It checks to see if the token has been passed to it,
 * additionally checking for the termination token. If a rank receives a black
 * token then it forwards a black token. Otherwise it forwards its own color.
 *
 * All nodes start out in the white state. State is *not* the same thing as
 * the token. If a node j sends work to a rank i (i < j) then its state turns
 * black. It then turns the token black when it comes around, forwards it, and
 * turns its state back to white.
 *
 * @param st the libcircle state struct.
 */
int32_t CIRCLE_check_for_term(CIRCLE_state_st* st)
{
    /* If I have the token (I am already idle) */
    if(st->have_token) {

        /* The master rank generates the original WHITE token */
        if(st->rank == 0) {

            //   LOG(CIRCLE_LOG_DBG, "Master generating WHITE token.");
            st->incoming_token = WHITE;

            MPI_Send(&st->incoming_token, 1, MPI_INT, \
                     (st->rank + 1) % st->size, \
                     TOKEN, *st->mpi_state_st->token_comm);

            st->token = WHITE;
            st->have_token = 0;

            /*
             * Immediately post a receive to listen for the token when it
             * comes back around
             */
            MPI_Irecv(&st->incoming_token, 1, MPI_INT, st->token_partner, \
                      TOKEN, *st->mpi_state_st->token_comm, \
                      &st->mpi_state_st->term_request);

            st->term_pending_receive = 1;
        }
        else {
            /*
             * In this case I am not the master rank.
             *
             * Turn it black if I am in a black state, and forward it since
             * I am idle.
             *
             * Then I turn my state white.
             */
            if(st->token == BLACK) {
                st->incoming_token = BLACK;
            }

            MPI_Send(&st->incoming_token, 1, MPI_INT, \
                     (st->rank + 1) % st->size, \
                     TOKEN, *st->mpi_state_st->token_comm);

            st->token = WHITE;
            st->have_token = 0;

            /*
             * Immediately post a receive to listen for the token when it
             * comes back around.
             */
            MPI_Irecv(&st->incoming_token, 1, MPI_INT, st->token_partner, \
                      TOKEN, *st->mpi_state_st->token_comm, \
                      &st->mpi_state_st->term_request);

            st->term_pending_receive = 1;
        }

        return 0;
    }
    /* If I don't have the token. */
    else {

        /* Check to see if I have posted a receive. */
        if(!st->term_pending_receive) {
            st->incoming_token = -1;

            MPI_Irecv(&st->incoming_token, 1, MPI_INT, st->token_partner, \
                      TOKEN, *st->mpi_state_st->token_comm, \
                      &st->mpi_state_st->term_request);

            st->term_pending_receive = 1;
        }

        st->term_flag = 0;

        /* Check to see if my pending receive has completed */
        MPI_Test(&st->mpi_state_st->term_request, &st->term_flag, \
                 &st->mpi_state_st->term_status);

        if(!st->term_flag) {
            return 0;
        }

        /* If I get here, then I received the token */
        st->term_pending_receive = 0;
        st->have_token = 1;

        // LOG(CIRCLE_LOG_DBG,"Received token %d\n",st->incoming_token);
        /* Check for termination */
        if(st->incoming_token == TERMINATE) {
            //  LOG(CIRCLE_LOG_DBG, "Received termination token");
            st->token = TERMINATE;
            MPI_Send(&st->token, 1, MPI_INT, (st->rank + 1) % st->size, \
                     TOKEN, *st->mpi_state_st->token_comm);

            //LOG(CIRCLE_LOG_DBG, "Forwared termination token");

            return TERMINATE;
        }

        if(st->token == BLACK && st->incoming_token == BLACK) {
            st->token = WHITE;
        }

        if(st->rank == 0 && st->incoming_token == WHITE) {
            LOG(CIRCLE_LOG_DBG, "Master has detected termination.");

            st->token = TERMINATE;

            MPI_Send(&st->token, 1, MPI_INT, 1, \
                     TOKEN, *st->mpi_state_st->token_comm);
            MPI_Send(&st->token, 1, MPI_INT, 1, \
                     WORK, *st->mpi_state_st->token_comm);

            return TERMINATE;
        }
    }

    return 0;
}

/**
 * This returns a random rank (not yourself).
 */
inline uint32_t CIRCLE_get_next_proc(uint32_t rank, uint32_t size)
{
    uint32_t result = rand() % size;

    while(result == rank) {
        result = rand() % size;
    }

    return result;
}

/**
 * @brief Waits on an incoming message.
 *
 * This function will spin lock waiting on a message to arrive from the
 * given source, with the given tag.  It does *not* receive the message.
 * If a message is pending, it will return it's size.  Otherwise
 * it returns 0.
 */
int32_t CIRCLE_wait_on_probe(CIRCLE_state_st* st, int32_t source, int32_t tag)
{
    int32_t flag = 0;
    uint32_t i = 0;
    MPI_Status temp;

    while(!flag) {
        MPI_Iprobe(source, tag, *st->mpi_state_st->work_comm, &flag, &temp);

        for(i = 0; i < st->size; i++) {
            st->request_flag[i] = 0;

            if(i != st->rank) {
                MPI_Test(&st->mpi_state_st->request_request[i], \
                         &st->request_flag[i], \
                         &st->mpi_state_st->request_status[i]);

                if(st->request_flag[i]) {
                    CIRCLE_send_no_work(i);
                    MPI_Start(&st->mpi_state_st->request_request[i]);
                }
            }
        }

        if(CIRCLE_check_for_term(st) == TERMINATE) {
            return TERMINATE;
        }
    }

    if(flag) {
        MPI_Get_count(&temp, MPI_INT, &flag);
        return flag;
    }
    else {
        return 0;
    }
}

/**
 * @brief Requests work from other ranks.
 *
 * Somewhat complicated, but essentially it requests work from a random
 * rank.  If it doesn't receive work, a different rank will be asked during
 * the next iteration.
 */
int32_t CIRCLE_request_work(CIRCLE_internal_queue_t* qp, CIRCLE_state_st* st)
{
    LOG(CIRCLE_LOG_DBG, "Sending work request to %d...", st->next_processor);

    int32_t temp_buffer = 3;

    if(CIRCLE_ABORT_FLAG) {
        temp_buffer = ABORT;
    }

    /* Send work request. */
    MPI_Send(&temp_buffer, 1, MPI_INT, st->next_processor, \
             WORK_REQUEST, *st->mpi_state_st->work_comm);

    st->work_offsets[0] = 0;

    /* Wait for an answer... */
    int32_t size = CIRCLE_wait_on_probe(st, st->next_processor, WORK);

    if(size == TERMINATE) {
        return TERMINATE;
    }

    if(size == 0) {
        st->next_processor = CIRCLE_get_next_proc(st->rank, st->size);
        return 0;
    }

    /*
     * If we get here, there was definitely an answer.
     * Receives the offsets then
     */
    MPI_Recv(st->work_offsets, size, MPI_INT, st->next_processor, \
             WORK, *st->mpi_state_st->work_comm, \
             &st->mpi_state_st->work_offsets_status);

    /* We'll ask somebody else next time */
    int32_t source = st->next_processor;

    st->next_processor = CIRCLE_get_next_proc(st->rank, st->size);

    int32_t chars = st->work_offsets[1];
    int32_t items = st->work_offsets[0];

    if(items == -1) {
        return -1;
    }
    else if(items == 0) {
        LOG(CIRCLE_LOG_DBG, "Received no work.");
        return 0;
    }
    else if(items == ABORT) {
        CIRCLE_ABORT_FLAG = 1;
        return ABORT;
    }

    /* Wait and see if they sent the work over */
    size = CIRCLE_wait_on_probe(st, source, WORK);

    if(size == TERMINATE) {
        return TERMINATE;
    }

    if(size == 0) {
        return 0;
    }

    /*
     * Good, we have a pending message from source with a WORK tag.
     * It can only be a work queue.
     */
    MPI_Recv(qp->base, (chars + 1) * sizeof(char), MPI_BYTE, source, WORK, \
             *st->mpi_state_st->work_comm, MPI_STATUS_IGNORE);

    qp->count = items;
    uint32_t i = 0;

    for(i = 0; i < qp->count; i++) {
        qp->strings[i] = qp->base + st->work_offsets[i + 2];
        //LOG(CIRCLE_LOG_DBG,
        //    "Item [%d] Offset [%d] String [%s]",
        //    i, st->work_offsets[i + 2], qp->strings[i]);
    }

    if(size == 0) {
        qp->count = 0;
        return 0;
    }

    if(qp->strings[0] != qp->base) {
        LOG(CIRCLE_LOG_FATAL, \
            "The base address of the queue doesn't match what it should be.");
        exit(EXIT_FAILURE);
    }

    qp->head = qp->strings[qp->count - 1] + strlen(qp->strings[qp->count - 1]);

    CIRCLE_internal_queue_print(qp);
    return 0;
}

/**
 * Sends a no work reply to someone requesting work.
 */
void CIRCLE_send_no_work(uint32_t dest)
{
    int32_t no_work[2];

    no_work[0] = (CIRCLE_ABORT_FLAG) ? ABORT : 0;
    no_work[1] = 0;

    MPI_Request r;
    MPI_Isend(&no_work, 1, MPI_INT, \
              dest, WORK, *CIRCLE_INPUT_ST.work_comm, &r);
    MPI_Wait(&r, MPI_STATUS_IGNORE);

}

/**
 * Distributes a random amount of the local work queue to the n requestors.
 */
void CIRCLE_send_work_to_many(CIRCLE_internal_queue_t* qp, \
                              CIRCLE_state_st* st, int* requestors, int32_t rcount)
{
    int32_t i = 0;

    if(rcount <= 0) {
        LOG(CIRCLE_LOG_FATAL,
            "Something is wrong with the amount of work we think we have.");
        exit(EXIT_FAILURE);
    }

    int32_t total_amount = rand() % (qp->count) + 1;

    /* Get size of chunk */
    int32_t increment = total_amount / rcount;

    for(i = 0; i < rcount; i ++) {
        total_amount -= increment;

        if(total_amount < increment) {
            increment += total_amount;
        }

        CIRCLE_send_work(qp, st, requestors[i], increment);
    }
}

/**
 * Sends work to a requestor
 */
int32_t CIRCLE_send_work(CIRCLE_internal_queue_t* qp, CIRCLE_state_st* st, \
                     int32_t dest, int32_t count)
{
    if(count <= 0) {
        CIRCLE_send_no_work(dest);
        return 0;
    }

    /* For termination detection */
    if((int)dest < (int)st->rank || (int)dest == (int)st->token_partner) {
        st->token = BLACK;
    }

    /* Base address of the buffer to be sent */
    char* b = qp->strings[qp->count - count];

    /* Address of the beginning of the last string to be sent */
    char* e = qp->strings[qp->count - 1];

    /* Distance between them */
    size_t diff = e - b;
    diff += strlen(e);

    if(qp->count < 10) {
        CIRCLE_internal_queue_print(qp);
    }

    /* offsets[0] = number of strings */
    /* offsets[1] = number of chars being sent */
    st->request_offsets[0] = count;
    st->request_offsets[1] = diff;

    if(diff >= (CIRCLE_INITIAL_INTERNAL_QUEUE_SIZE * CIRCLE_MAX_STRING_LEN)) {
        LOG(CIRCLE_LOG_FATAL, \
            "We're trying to throw away part of the queue for some reason.");
        exit(EXIT_FAILURE);
    }

    int32_t j = qp->count - count;
    int32_t i = 0;

    for(i = 0; i < (int32_t)st->request_offsets[0]; i++) {
        st->request_offsets[i + 2] = qp->strings[j++] - b;
    }

    /* offsets[qp->count - qp->count/2+2]  is the size of the last string */
    st->request_offsets[count + 2] = strlen(qp->strings[qp->count - 1]);


    MPI_Ssend(st->request_offsets, st->request_offsets[0] + 2, \
              MPI_INT, dest, WORK, *st->mpi_state_st->work_comm);


    MPI_Ssend(b, (diff + 1) * sizeof(char), MPI_BYTE, \
              dest, WORK, *st->mpi_state_st->work_comm);

    qp->count = qp->count - count;
    //LOG(CIRCLE_LOG_DBG,
    //    "Sent %d items to %d.", st->request_offsets[0], dest);

    return 0;
}

/**
 * Checks for outstanding work requests
 */
int32_t CIRCLE_check_for_requests(CIRCLE_internal_queue_t* qp, CIRCLE_state_st* st)
{
    int* requestors = (int*)calloc(st->size, sizeof(int));
    uint32_t i = 0;
    uint32_t rcount = 0;
    /* This loop is only excuted once.  It is used to initiate receives.
     * When a received is completed, we repost it immediately to capture
     * the next request */
    if(!st->request_pending_receive) {
        for(i = 0; i < st->size; i++) {
            if(i != st->rank) {
                st->request_recv_buf[i] = 0;
                MPI_Recv_init(&st->request_recv_buf[i], 1, MPI_INT, i, \
                              WORK_REQUEST, *st->mpi_state_st->work_comm, \
                              &st->mpi_state_st->request_request[i]);

                MPI_Start(&st->mpi_state_st->request_request[i]);
            }
        }

        st->request_pending_receive = 1;
    }

    /* Test to see if any posted receive has completed */
    for(i = 0; i < st->size; i++) {
        if(i != st->rank) {
            st->request_flag[i] = 0;

            if(MPI_Test(&st->mpi_state_st->request_request[i], \
                        &st->request_flag[i], \
                        &st->mpi_state_st->request_status[i]) != MPI_SUCCESS)
                { exit(EXIT_FAILURE); }

            if(st->request_flag[i]) {
                if(st->request_recv_buf[i] == ABORT) {
                    CIRCLE_ABORT_FLAG = 1;
                    return ABORT;
                }

//                LOG(CIRCLE_LOG_DBG,"Received work request from %d\n",i);
                requestors[rcount++] = i;
                st->request_flag[i] = 0;
            }
        }
    }

    /* If we didn't received any work request, no need to continue */
    if(rcount == 0) {
        return 0;
    }

    if(qp->count == 0 || CIRCLE_ABORT_FLAG) {
        for(i = 0; i < rcount; i++) {
            CIRCLE_send_no_work(requestors[i]);
        }
    }
    /*
     * If you get here, then you have work to send AND the CIRCLE_ABORT_FLAG
     * is not set
     */
    else {
        CIRCLE_send_work_to_many(qp, st, requestors, rcount);
    }

    for(i = 0; i < rcount; i++) {
        MPI_Start(&st->mpi_state_st->request_request[requestors[i]]);
    }

    free(requestors);

    return 0;
}

/**
 * Print the offsets of a copied queue.
 */
void CIRCLE_print_offsets(uint32_t* offsets, int32_t count)
{
    int32_t i = 0;

    for(i = 0; i < count; i++) {
        LOG(CIRCLE_LOG_DBG, "\t[%d] %d", i, offsets[i]);
    }
}

/* EOF */

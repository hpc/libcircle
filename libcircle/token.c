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
#include <assert.h>

#include "libcircle.h"
#include "log.h"
#include "token.h"
#include "worker.h"
#include "queue.h"

extern int8_t CIRCLE_ABORT_FLAG;
extern uint32_t local_work_requested;
extern uint32_t local_no_work_received;

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
        /* no need to go through the messaging
         * if we're the only rank in the job */
        if(st->size == 1) {
            /* TODO: need to set any other fields? */
            st->token = TERMINATE;
            return TERMINATE;
        }

        /* The master rank generates the original WHITE token */
        if(st->rank == 0) {

            //   LOG(CIRCLE_LOG_DBG, "Master generating WHITE token.");
            st->incoming_token = WHITE;

            MPI_Send(&st->incoming_token, 1, MPI_INT, \
                     st->token_partner_send, \
                     TOKEN, *st->mpi_state_st->token_comm);

            st->token = WHITE;
            st->have_token = 0;

            /*
             * Immediately post a receive to listen for the token when it
             * comes back around
             */
            MPI_Irecv(&st->incoming_token, 1, MPI_INT, st->token_partner_recv, \
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
                     st->token_partner_send, \
                     TOKEN, *st->mpi_state_st->token_comm);

            st->token = WHITE;
            st->have_token = 0;

            /*
             * Immediately post a receive to listen for the token when it
             * comes back around.
             */
            MPI_Irecv(&st->incoming_token, 1, MPI_INT, st->token_partner_recv, \
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

            MPI_Irecv(&st->incoming_token, 1, MPI_INT, st->token_partner_recv, \
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
            MPI_Send(&st->token, 1, MPI_INT, st->token_partner_send, \
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

            return TERMINATE;
        }
    }

    return 0;
}

/**
 * This returns a rank (not yourself).
 */
inline void
CIRCLE_get_next_proc(CIRCLE_state_st* st)
{
    do {
        st->next_processor = rand_r(&st->seed) % st->size;
    }
    while(st->next_processor == st->rank);
}

/**
 * @brief Waits on an incoming message.
 *
 * This function will spin lock waiting on a message to arrive from the
 * given source, with the given tag.  It does *not* receive the message.
 * If a message is pending, it will return it's size.  Otherwise
 * it returns 0.
 */
void CIRCLE_wait_on_probe(CIRCLE_state_st* st, int32_t source, int32_t tag, int* terminate, int* msg, MPI_Status* status)
{
    /* mark both flags to zero to start with */
    *terminate = 0;
    *msg = 0;

    /* loop until we get a message on work communicator or a terminate signal */
    int done = 0;

    while(! done) {
        int flag;
        MPI_Iprobe(source, tag, *st->mpi_state_st->work_comm, &flag, status);

        uint32_t i = 0;

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
            *terminate = 1;
            done = 1;
        }

        if(flag) {
            *msg = 1;
            done = 1;
        }
    }

    return;
}

/**
 * @brief Extend the offset arrays.
 */
int8_t CIRCLE_extend_offsets(CIRCLE_state_st* st, uint32_t size)
{
    uint32_t count = st->offset_count;

    while(count < size) {
        count += 4096;
    }

    LOG(CIRCLE_LOG_DBG, "Extending offset arrays from %d to %d.", \
        st->offset_count, count);

    st->work_offsets = (uint32_t*) realloc(st->work_offsets, \
                                           count * sizeof(uint32_t));
    st->request_offsets = (uint32_t*) realloc(st->request_offsets, \
                          count * sizeof(uint32_t));

    LOG(CIRCLE_LOG_DBG, "Work offsets: [%p] -> [%p]", \
        (void*) st->work_offsets, \
        (void*)(st->work_offsets + (count * sizeof(uint32_t))));

    LOG(CIRCLE_LOG_DBG, "Request offsets: [%p] -> [%p]", \
        (void*) st->request_offsets, \
        (void*)(st->request_offsets + (count * sizeof(uint32_t))));

    st->offset_count = count;

    if(!st->work_offsets || !st->request_offsets) {
        return -1;
    }

    return 0;
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
    local_work_requested++;
    int32_t temp_buffer = 3;

    if(CIRCLE_ABORT_FLAG) {
        temp_buffer = ABORT;
    }

    /* Send work request. */
    MPI_Send(&temp_buffer, 1, MPI_INT, st->next_processor, \
             WORK_REQUEST, *st->mpi_state_st->work_comm);

    /* Count cost of message
     * one integer * hops
     */
    st->work_offsets[0] = 0;

    /* Wait for an answer... */
    int terminate, msg;
    MPI_Status status;
    CIRCLE_wait_on_probe(st, st->next_processor, WORK, &terminate, &msg, &status);

    if(terminate) {
        return TERMINATE;
    }

    int size;
    MPI_Get_count(&status, MPI_INT, &size);

    if(size == 0) {
        LOG(CIRCLE_LOG_FATAL,
            "size == 0.");
        MPI_Abort(0, MPI_COMM_WORLD);
        return 0;
    }

    /* Check to see if the offset array is large enough */
    if(size >= (signed)st->offset_count) {
        if(CIRCLE_extend_offsets(st, size) < 0) {
            LOG(CIRCLE_LOG_ERR, "Error: Unable to extend offsets.");
            return -1;
        }
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

    CIRCLE_get_next_proc(st);

    int32_t items = st->work_offsets[0];
    int32_t chars = st->work_offsets[1];

    if(items == -1) {
        return -1;
    }
    else if(items == 0) {
        LOG(CIRCLE_LOG_DBG, "Received no work.");
        local_no_work_received++;
        return 0;
    }
    else if(items == ABORT) {
        CIRCLE_ABORT_FLAG = 1;
        return ABORT;
    }

    /* Make sure our queue is large enough */
    while((qp->base + chars + 1) > qp->end) {
        if(CIRCLE_internal_queue_extend(qp) < 0) {
            LOG(CIRCLE_LOG_ERR, "Error: Unable to realloc string pool.");
            return -1;
        }
    }

    /* Make sure the queue string array is large enough */
    if(items > (signed)qp->str_count) {
        if(CIRCLE_internal_queue_str_extend(qp, items) < 0) {
            LOG(CIRCLE_LOG_ERR, "Error: Unable to realloc string array.");
            return -1;
        }
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
        qp->strings[i] = st->work_offsets[i + 2];
    }

    if(qp->strings[0] != 0) {
        LOG(CIRCLE_LOG_FATAL, \
            "The base address of the queue doesn't match what it should be.");
        exit(EXIT_FAILURE);
    }

    qp->head = qp->strings[qp->count - 1] + strlen(qp->base + qp->strings[qp->count - 1]);
    LOG(CIRCLE_LOG_DBG, "Received %d items from %d", qp->count, source);

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

/* spread count equally among ranks, handle cases where number
 * of ranks doesn't evenly divide remaining count by scattering
 * remainder across initial ranks */
static void spread_counts(int* sizes, int ranks, int count)
{
    int32_t i = 0;
    int32_t base  = count / ranks;
    int32_t extra = count - base * ranks;

    while(i < extra) {
        sizes[i] = base + 1;
        i++;
    }

    while(i < ranks) {
        sizes[i] = base;
        i++;
    }

    return;
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

    /* TODO: could allocate this once up front during init */
    /* we have rcount requestors and ourself, allocate array to store
     * number of elements we'll send to each, storing the amount we
     * keep as the first entry */
    int num_ranks = rcount + 1;
    int* sizes = (int*) malloc(num_ranks * sizeof(int));

    if(sizes == NULL) {
        LOG(CIRCLE_LOG_FATAL,
            "Failed to allocate memory for sizes.");
        MPI_Abort(0, MPI_COMM_WORLD);
    }

    if(CIRCLE_INPUT_ST.options & CIRCLE_SPLIT_EQUAL) {
        /* split queue equally among ourself and all requestors */
        spread_counts(&sizes[0], num_ranks, qp->count);
    }
    else { /* CIRCLE_SPLIT_RANDOM */
        /* randomly pick a total amount to send to requestors,
         * but keep at least one item */
        int send_count = (rand_r(&st->seed) % qp->count) + 1;

        if(send_count == qp->count) {
            send_count--;
        }

        /* we keep the first portion, and spread the rest */
        sizes[0] = qp->count - send_count;
        spread_counts(&sizes[1], rcount, send_count);
    }

    /* send elements to requestors, note the requestor array
     * starts at 0 and sizes start at 1 */
    for(i = 0; i < rcount; i ++) {
        CIRCLE_send_work(qp, st, requestors[i], sizes[i + 1]);
    }

    free(sizes);

    LOG(CIRCLE_LOG_DBG, "Done servicing requests.");
}

/**
 * Sends work to a requestor
 */
int32_t CIRCLE_send_work(CIRCLE_internal_queue_t* qp, CIRCLE_state_st* st, \
                         int32_t dest, int32_t count)
{
    if(count <= 0) {
        CIRCLE_send_no_work(dest);
        /* Add cost of message */
        return 0;
    }

    /* For termination detection */
    if((int)dest < (int)st->rank || (int)dest == (int)st->token_partner_recv) {
        st->token = BLACK;
    }

    /* Base address of the buffer to be sent */
    uintptr_t b = qp->strings[qp->count - count];

    /* Address of the beginning of the last string to be sent */
    uintptr_t e = qp->strings[qp->count - 1];

    /* Distance between them */
    size_t diff = e - b;
    diff += strlen(qp->base + e);

    /* Check to see if the offset array is large enough */
    if(count >= (signed)st->offset_count) {
        if(CIRCLE_extend_offsets(st, count) < 0) {
            LOG(CIRCLE_LOG_ERR, "Error: Unable to extend offsets.");
            return -1;
        }
    }

    /* offsets[0] = number of strings */
    /* offsets[1] = number of chars being sent */
    st->request_offsets[0] = count;
    st->request_offsets[1] = diff;

    int32_t j = qp->count - count;
    int32_t i = 0;

    for(i = 0; i < (int32_t)st->request_offsets[0]; i++) {
        st->request_offsets[i + 2] = qp->strings[j++] - b;
    }

    /* offsets[qp->count - qp->count/2+2]  is the size of the last string */
    st->request_offsets[count + 2] = strlen(qp->base + qp->strings[qp->count - 1]);

    MPI_Ssend(st->request_offsets, st->request_offsets[0] + 2, \
              MPI_INT, dest, WORK, *st->mpi_state_st->work_comm);

    MPI_Ssend(qp->base + b, (diff + 1) * sizeof(char), MPI_BYTE, \
              dest, WORK, *st->mpi_state_st->work_comm);

    LOG(CIRCLE_LOG_DBG,
        "Sent %d of %d items to %d.", st->request_offsets[0], qp->count, dest);
    qp->count = qp->count - count;

    return 0;
}

/**
 * Checks for outstanding work requests
 */
int32_t CIRCLE_check_for_requests(CIRCLE_internal_queue_t* qp, CIRCLE_state_st* st)
{
    int* requestors = (int*)calloc(st->size, sizeof(*requestors));
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
                        &st->mpi_state_st->request_status[i]) != MPI_SUCCESS) {
                exit(EXIT_FAILURE);
            }

            if(st->request_flag[i]) {
                if(st->request_recv_buf[i] == ABORT) {
                    CIRCLE_ABORT_FLAG = 1;
                    return ABORT;
                }

                LOG(CIRCLE_LOG_DBG, "Received work request from %d", i);
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

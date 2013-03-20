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
    if(st->size > 1) {
        do {
            st->next_processor = rand_r(&st->seed) % st->size;
        }
        while(st->next_processor == st->rank);
    }
    else {
        /* for a job size of one, we have no one to ask */
        st->next_processor = MPI_PROC_NULL;
    }
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

        int32_t i = 0;

        for(i = 0; i < st->size; i++) {
            st->request_flag[i] = 0;

            if(i != st->rank) {
                MPI_Test(&st->mpi_state_st->request_request[i], \
                         &st->request_flag[i], \
                         &st->mpi_state_st->request_status[i]);

                if(st->request_flag[i]) {
                    MPI_Start(&st->mpi_state_st->request_request[i]);
                    CIRCLE_send_no_work(i);
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
int8_t CIRCLE_extend_offsets(CIRCLE_state_st* st, int32_t size)
{
    /* get current size of offset arrays */
    int32_t count = st->offset_count;

    /* if size we need is less than or equal to current size,
     * we don't need to do anything */
    if(size <= count) {
        return 0;
    }

    /* otherwise, allocate more in blocks of 4096 at a time */
    while(count < size) {
        count += 4096;
    }

    LOG(CIRCLE_LOG_DBG, "Extending offset arrays from %d to %d.", \
        st->offset_count, count);

    st->work_offsets = (int*) realloc(st->work_offsets, \
                                      (size_t)count * sizeof(int));
    st->request_offsets = (int*) realloc(st->request_offsets, \
                                         (size_t)count * sizeof(int));

    LOG(CIRCLE_LOG_DBG, "Work offsets: [%p] -> [%p]", \
        (void*) st->work_offsets, \
        (void*)(st->work_offsets + ((size_t)count * sizeof(int))));

    LOG(CIRCLE_LOG_DBG, "Request offsets: [%p] -> [%p]", \
        (void*) st->request_offsets, \
        (void*)(st->request_offsets + ((size_t)count * sizeof(int))));

    /* record new length of offset arrays */
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
    /* get rank of process to request work from */
    int32_t source = st->next_processor;

    /* have no one to ask, we're done */
    if(source == MPI_PROC_NULL) {
        /* initiate termination */
        if(CIRCLE_check_for_term(st) != TERMINATE) {
            LOG(CIRCLE_LOG_FATAL, "Expected to terminate but did not.");
            MPI_Abort(*st->mpi_state_st->work_comm, LIBCIRCLE_MPI_ERROR);
        }

        return TERMINATE;
    }

    LOG(CIRCLE_LOG_DBG, "Sending work request to %d...", source);
    local_work_requested++;
    int32_t temp_buffer = 3;

    if(CIRCLE_ABORT_FLAG) {
        temp_buffer = ABORT;
    }

    /* Send work request. */
    MPI_Send(&temp_buffer, 1, MPI_INT, source, \
             WORK_REQUEST, *st->mpi_state_st->work_comm);

    /* Count cost of message
     * one integer * hops
     */
    st->work_offsets[0] = 0;

    /* Wait for an answer... */
    int terminate, msg;
    MPI_Status status;
    CIRCLE_wait_on_probe(st, source, WORK, &terminate, &msg, &status);

    if(terminate) {
        return TERMINATE;
    }

    int size;
    MPI_Get_count(&status, MPI_INT, &size);

    if(size <= 0) {
        LOG(CIRCLE_LOG_FATAL, "size <= 0.");
        MPI_Abort(*st->mpi_state_st->work_comm, LIBCIRCLE_MPI_ERROR);
        return 0;
    }

    /* Check to see if the offset array is large enough */
    if(CIRCLE_extend_offsets(st, size) < 0) {
        LOG(CIRCLE_LOG_ERR, "Error: Unable to extend offsets.");
        return -1;
    }

    /*
     * If we get here, there was definitely an answer.
     * Receives the offsets then
     */
    MPI_Recv(st->work_offsets, size, MPI_INT, source, \
             WORK, *st->mpi_state_st->work_comm, \
             &st->mpi_state_st->work_offsets_status);

    /* We'll ask somebody else next time */
    CIRCLE_get_next_proc(st);

    int items = st->work_offsets[0];
    int chars = st->work_offsets[1];

    if(items == 0) {
        LOG(CIRCLE_LOG_DBG, "Received no work.");
        local_no_work_received++;
        return 0;
    }
    else if(items == ABORT) {
        CIRCLE_ABORT_FLAG = 1;
        return ABORT;
    }
    else if(items < 0) {
        return -1;
    }

    /* Make sure our queue is large enough */
    while((qp->base + chars) > qp->end) {
        if(CIRCLE_internal_queue_extend(qp) < 0) {
            LOG(CIRCLE_LOG_ERR, "Error: Unable to realloc string pool.");
            return -1;
        }
    }

    /* Make sure the queue string array is large enough */
    int32_t count = items;

    if(count > qp->str_count) {
        if(CIRCLE_internal_queue_str_extend(qp, count) < 0) {
            LOG(CIRCLE_LOG_ERR, "Error: Unable to realloc string array.");
            return -1;
        }
    }

    /*
     * Good, we have a pending message from source with a WORK tag.
     * It can only be a work queue.
     */
    MPI_Recv(qp->base, chars, MPI_BYTE, source, WORK, \
             *st->mpi_state_st->work_comm, MPI_STATUS_IGNORE);

    int32_t i = 0;
    qp->count = count;

    for(i = 0; i < count; i++) {
        qp->strings[i] = (uintptr_t) st->work_offsets[i + 2];
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
void CIRCLE_send_no_work(int32_t dest)
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
                              CIRCLE_state_st* st, int* requestors, int rcount)
{
    int i = 0;

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
    int* sizes = (int*) malloc((size_t)num_ranks * sizeof(int));

    if(sizes == NULL) {
        LOG(CIRCLE_LOG_FATAL,
            "Failed to allocate memory for sizes.");
        MPI_Abort(*st->mpi_state_st->work_comm, LIBCIRCLE_MPI_ERROR);
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
                         int dest, int32_t count)
{
    if(count <= 0) {
        CIRCLE_send_no_work(dest);
        /* Add cost of message */
        return 0;
    }

    /* For termination detection */
    if(dest < st->rank || dest == st->token_partner_recv) {
        st->token = BLACK;
    }

    /* Base address of the buffer to be sent */
    int32_t start_elem = qp->count - count;
    uintptr_t start_offset = qp->strings[start_elem];

    /* Address of the beginning of the last string to be sent */
    int32_t end_elem = qp->count - 1;
    uintptr_t end_offset = qp->strings[end_elem];

    /* Distance between them */
    size_t len = end_offset - start_offset;
    len += strlen(qp->base + end_offset) + 1;

    /* TODO: check that len doesn't overflow an int */
    int bytes = (int) len;

    /* total number of ints we'll send */
    int numoffsets = 2 + count;

    /* Check to see if the offset array is large enough */
    if(CIRCLE_extend_offsets(st, numoffsets) < 0) {
        LOG(CIRCLE_LOG_ERR, "Error: Unable to extend offsets.");
        return -1;
    }

    /* offsets[0] = number of strings */
    /* offsets[1] = number of chars being sent */
    st->request_offsets[0] = (int) count;
    st->request_offsets[1] = (int) bytes;

    /* now compute offset of each string */
    int32_t i = 0;
    int32_t current_elem = start_elem;

    for(i = 0; i < count; i++) {
        st->request_offsets[2 + i] = (int)(qp->strings[current_elem] - start_offset);
        current_elem++;
    }

    /* send item count, total bytes, and offsets of each item */
    MPI_Ssend(st->request_offsets, numoffsets, \
              MPI_INT, dest, WORK, *st->mpi_state_st->work_comm);

    /* send data */
    char* buf = qp->base + start_offset;
    MPI_Ssend(buf, bytes, MPI_BYTE, \
              dest, WORK, *st->mpi_state_st->work_comm);

    LOG(CIRCLE_LOG_DBG,
        "Sent %d of %d items to %d.", st->request_offsets[0], qp->count, dest);

    /* subtract elements from our queue */
    qp->count = qp->count - count;

    return 0;
}

/**
 * Checks for outstanding work requests
 */
int32_t CIRCLE_check_for_requests(CIRCLE_internal_queue_t* qp, CIRCLE_state_st* st)
{
    int i = 0;

    /* record list of requesting ranks in requestors
     * and number in rcount */
    int* requestors = st->mpi_state_st->requestors;
    int rcount = 0;

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

    /* reset our receives before we send work to requestors */
    for(i = 0; i < rcount; i++) {
        MPI_Start(&st->mpi_state_st->request_request[requestors[i]]);
    }

    /* send work to requestors */
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

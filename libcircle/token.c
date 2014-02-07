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

extern CIRCLE_input_st CIRCLE_INPUT_ST;

/* given the process's rank and the number of ranks, this computes a k-ary
 * tree rooted at rank 0, the structure records the number of children
 * of the local rank and the list of their ranks */
void CIRCLE_tree_init(int32_t rank, int32_t ranks, int32_t k, MPI_Comm comm, CIRCLE_tree_state_st* t)
{
    int32_t i;

    /* initialize fields */
    t->rank        = (int) rank;
    t->ranks       = (int) ranks;
    t->parent_rank = MPI_PROC_NULL;
    t->children    = 0;
    t->child_ranks = NULL;

    /* compute the maximum number of children this task may have */
    int32_t max_children = k;

    /* allocate memory to hold list of children ranks */
    if(max_children > 0) {
        size_t bytes = (size_t)max_children * sizeof(int);
        t->child_ranks = (int*) malloc(bytes);

        if(t->child_ranks == NULL) {
            LOG(CIRCLE_LOG_FATAL,
                "Failed to allocate memory for list of children.");
            MPI_Abort(comm, LIBCIRCLE_MPI_ERROR);
        }
    }

    /* initialize all ranks to NULL */
    for(i = 0; i < max_children; i++) {
        t->child_ranks[i] = MPI_PROC_NULL;
    }

    /* compute rank of our parent if we have one */
    if (rank > 0) {
        t->parent_rank = (rank - 1) / k;
    }

    /* identify ranks of what would be leftmost and rightmost children */
    int32_t left  = rank * k + 1;
    int32_t right = rank * k + k;

    /* if we have at least one child,
     * compute number of children and list of child ranks */
    if (left < ranks) {
        /* adjust right child in case we don't have a full set of k */
        if (right >= ranks) {
            right = ranks - 1;
        }

        /* compute number of children and list of child ranks */
        t->children = (int) (right - left + 1);
        for(i = 0; i < t->children; i++) {
            t->child_ranks[i] = (int) (left + i);
        }
    }

    return;
}

void CIRCLE_tree_free(CIRCLE_tree_state_st* t)
{
    /* free child rank list */
    if (t->child_ranks != NULL) {
        free(t->child_ranks);
        t->child_ranks = NULL;
    }

    return;
}

/* intiates and progresses a reduce operation at specified interval,
 * ensures progress of reduction in background */
void CIRCLE_reduce_progress(CIRCLE_state_st* st, int count)
{
    int i;
    int flag;
    MPI_Status status;

    /* get our communicator */
    MPI_Comm comm = *(st->mpi_state_st->work_comm);

    /* get info about tree */
    int  parent_rank = st->tree.parent_rank;
    int  children    = st->tree.children;
    int* child_ranks = st->tree.child_ranks;

    /* if we have an outstanding reduce, check messages from children,
     * otherwise, check whether we should start a new reduce */
    if(st->reduce_outstanding) {
        /* got a reduce outstanding, check messages from our children */
        for(i = 0; i < children; i++) {
            /* pick a child */
            int child = child_ranks[i];

            /* check whether this child has sent us a reduce message */
            MPI_Iprobe(child, CIRCLE_TAG_REDUCE, comm, &flag, &status);

            /* if we got a message, receive and reduce it */
            if(flag) {
                /* receive message form child */
                int recvbuf;
                MPI_Recv(&recvbuf, 1, MPI_INT, child, CIRCLE_TAG_REDUCE, comm, &status);

                /* combine child's data with our buffer */
                st->reduce_buf += recvbuf;

                /* increment the number of replies */
                st->reduce_replies++;
            }
        }

        /* check whether we've gotten replies from all children */
        if(st->reduce_replies == children) {
            /* add our own content to reduce buffer */
            st->reduce_buf += count;

            /* send message to parent if all children have replied */
            if(parent_rank != MPI_PROC_NULL) {
                MPI_Send(&st->reduce_buf, 1, MPI_INT, parent_rank, CIRCLE_TAG_REDUCE, comm);
            } else {
                /* we're the root, print the results now */
                fprintf(CIRCLE_debug_stream, "Objects processed: %d\n", st->reduce_buf);
                fflush(CIRCLE_debug_stream);
            }

            /* disable flag that indicates we have an outstanding reduce */
            st->reduce_outstanding = 0;
        }
    } else {
        /* determine whether a new reduce should be started, only bother
         * checking if we think it's about time */
        int start_reduce = 0;
        double time_now = MPI_Wtime();
        double time_next = st->reduce_time_last + st->reduce_time_interval;
        if(time_now >= time_next) {
            /* time has expired, new reduce should be started */
            if(parent_rank == MPI_PROC_NULL) {
                /* we're the root, kick it off */
                start_reduce = 1;
            } else {
                /* we're not the root, check whether parent sent us a message */
                MPI_Iprobe(parent_rank, CIRCLE_TAG_REDUCE, comm, &flag, &status);

                /* kick off reduce if message came in */
                if(flag) {
                    /* receive message from parent and set flag to start reduce */
                    MPI_Recv(NULL, 0, MPI_BYTE, parent_rank, CIRCLE_TAG_REDUCE, comm, &status);
                    start_reduce = 1;
                }
            }
        }

        /* kick off a reduce if it's time */
        if (start_reduce) {
            /* set flag to indicate we have a reduce outstanding
             * and initialize state for a fresh reduction */
            st->reduce_time_last   = time_now;
            st->reduce_outstanding = 1;
            st->reduce_replies     = 0;
            st->reduce_buf         = 0;

            /* send message to each child */
            /* check for messages from our children */
            for (i = 0; i < children; i++) {
                /* send message to each child */
                int child = child_ranks[i];
                MPI_Send(NULL, 0, MPI_BYTE, child, CIRCLE_TAG_REDUCE, comm);
            }
        }
    }

    return;
}

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
    /* get communicator */
    MPI_Comm comm = st->token_comm;

    /* If I have the token (I am already idle) */
    if(st->token_flag) {
        /* no need to go through the messaging
         * if we're the only rank in the job */
        if(st->size == 1) {
            /* TODO: need to set any other fields? */
            st->token = TERMINATE;
            return TERMINATE;
        }

        /* get current token color */
        int token = st->token_recv_buf;

        /* Immediately post a receive to listen for the token when it
         * comes back around */
        MPI_Irecv(&st->token_recv_buf, 1, MPI_INT, st->token_partner_recv,
                  CIRCLE_TAG_TOKEN, comm, &st->token_recv_req);
        st->token_recv_pending = 1;

        /* change token color depending on our rank and state */
        if(st->rank == 0) {
            /* The master rank generates the original WHITE token */
            token = WHITE;
        } else if (st->token == BLACK) {
            /* Others turn the token black if they are in the black
             * state */
             token = BLACK;
        }

        /* TODO: change this to non-blocking to avoid deadlock */

        /* send the token */
        MPI_Send(&token, 1, MPI_INT, st->token_partner_send,
                 CIRCLE_TAG_TOKEN, comm);

        /* flip our color back to white and record that we sent
         * the token */
        st->token = WHITE;
        st->token_flag = 0;

        return 0;
    }
    /* If I don't have the token. */
    else {
        /* post a receive for the token if we haven't already */
        if(! st->token_recv_pending) {
            MPI_Irecv(&st->token_recv_buf, 1, MPI_INT, st->token_partner_recv,
                      CIRCLE_TAG_TOKEN, comm, &st->token_recv_req);
            st->token_recv_pending = 1;
        }

        /* Check to see if my pending receive has completed */
        int flag;
        MPI_Status status;
        MPI_Test(&st->token_recv_req, &flag, &status);

        /* still don't have the token, return */
        if(! flag) {
            return 0;
        }

        /* If I get here, then I received the token */
        st->token_recv_pending = 0;
        st->token_flag = 1;

        /* what's this logic account for? */
        if(st->token == BLACK && st->token_recv_buf == BLACK) {
            st->token = WHITE;
        }

        /* check for termination */
        int terminate = 0;
        if(st->rank == 0 && st->token_recv_buf == WHITE) {
            /* if a white token makes it back to rank 0,
             * we start the termination token */
            LOG(CIRCLE_LOG_DBG, "Master has detected termination.");
            terminate = 1;
        } else if (st->token_recv_buf == TERMINATE) {
            /* if we're not rank 0, we just look for the terminate token */
            terminate = 1;
        }

        /* forward the terminate token if we have one */
        if (terminate) {
            /* TODO: change this to non-blocking to avoid deadlock */
            /* set state to TERMINATE, and forward the token */
            st->token = TERMINATE;
            MPI_Send(&st->token, 1, MPI_INT, st->token_partner_send,
                     CIRCLE_TAG_TOKEN, comm);
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

/* we execute this function when we have detected incoming work messages */
static int32_t CIRCLE_work_receive(
    CIRCLE_internal_queue_t* qp,
    CIRCLE_state_st* st,
    int source,
    int size)
{
    /* get communicator */
    MPI_Comm comm = *st->mpi_state_st->work_comm;

    /* this shouldn't happen, but let's check so we don't blow out
     * memory allocation below */
    if(size <= 0) {
        LOG(CIRCLE_LOG_FATAL, "size <= 0.");
        MPI_Abort(comm, LIBCIRCLE_MPI_ERROR);
        return -1;
    }

    /* Check to see if the offset array is large enough */
    if(CIRCLE_extend_offsets(st, size) < 0) {
        LOG(CIRCLE_LOG_ERR, "Error: Unable to extend offsets.");
        MPI_Abort(comm, LIBCIRCLE_MPI_ERROR);
        return -1;
    }

    /* Receive item count, character count, and offsets */
    MPI_Recv(st->work_offsets, size, MPI_INT, source,
             WORK, comm, &st->mpi_state_st->work_offsets_status);

    /* the first int has number of items or an ABORT code */
    int items = st->work_offsets[0];
    if(items == 0) {
        /* we received 0 elements, there is no follow on message */
        LOG(CIRCLE_LOG_DBG, "Received no work.");
        st->local_no_work_received++;
        return 0;
    }
    else if(items == ABORT) {
        /* we've received a signal to kill the job,
         * there is no follow on message in this case */
        CIRCLE_ABORT_FLAG = 1;
        return ABORT;
    }
    else if(items < 0) {
        /* TODO: when does this happen? */
        return -1;
    }

    /* the second int is the number of characters we'll receive,
     * make sure our queue has enough storage */
    int chars = st->work_offsets[1];
    size_t new_bytes = (size_t)(qp->head + chars) * sizeof(char);
    if(new_bytes > qp->bytes) {
        if(CIRCLE_internal_queue_extend(qp, new_bytes) < 0) {
            LOG(CIRCLE_LOG_ERR, "Error: Unable to realloc string pool.");
            MPI_Abort(comm, LIBCIRCLE_MPI_ERROR);
            return -1;
        }
    }

    /* receive second message containing work elements */
    MPI_Recv(qp->base, chars, MPI_CHAR, source, WORK,
             comm, MPI_STATUS_IGNORE);

    /* make sure we have a pointer allocated for each element */
    int32_t count = items;
    if(count > qp->str_count) {
        if(CIRCLE_internal_queue_str_extend(qp, count) < 0) {
            LOG(CIRCLE_LOG_ERR, "Error: Unable to realloc string array.");
            MPI_Abort(comm, LIBCIRCLE_MPI_ERROR);
            return -1;
        }
    }

    /* set offset to each element in our queue */
    int32_t i;
    for(i = 0; i < count; i++) {
        qp->strings[i] = (uintptr_t) st->work_offsets[i + 2];
    }

    /* double check that the base offset is valid */
    if(qp->strings[0] != 0) {
        LOG(CIRCLE_LOG_FATAL, \
            "The base address of the queue doesn't match what it should be.");
        MPI_Abort(comm, LIBCIRCLE_MPI_ERROR);
        return -1;
    }

    /* we now have count items in our queue */
    qp->count = count;

    /* set head of queue to point just past end of last element string */
    uintptr_t elem_offset = qp->strings[count - 1];
    const char* elem_str = qp->base + elem_offset;
    size_t elem_len = strlen(elem_str) + 1;
    qp->head = elem_offset + elem_len;

    /* log number of items we received */
    LOG(CIRCLE_LOG_DBG, "Received %d items from %d", count, source);

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
    int rc = 0;

    /* get communicator */
    MPI_Comm comm = *st->mpi_state_st->work_comm;

    /* check whether we've already sent a request or not */
    if(! st->work_requested) {
        /* need to send request, get rank of process to request work from */
        int32_t source = st->next_processor;

        /* have no one to ask, we're done */
        if(source == MPI_PROC_NULL) {
            return rc;
        }

        LOG(CIRCLE_LOG_DBG, "Sending work request to %d...", source);

        /* increment number of work requests */
        st->local_work_requested++;

        /* TODO: why 3? */
        /* if abort flag is set, pack abort code into send buffer */
        int32_t buf = 3;
        if(CIRCLE_ABORT_FLAG) {
            buf = ABORT;
        }

        /* send work request (we use isend to avoid deadlock) */
        MPI_Isend(&buf, 1, MPI_INT, source, WORK_REQUEST,
            comm, &st->work_requested_req);

        /* set flag and source to indicate we requested work */
        st->work_requested = 1;
        st->work_requested_rank = source;

        /* randomly pick another source to ask next time */
        CIRCLE_get_next_proc(st);
    } else {
        /* get source rank */
        int source = st->work_requested_rank;

        /* we've already requested work from someone, check whether
         * we got a reply */
        int flag;
        MPI_Status status;
        MPI_Iprobe(source, WORK, comm, &flag, &status);

        /* if we got a reply, process it */
        if(flag) {
            /* get number of integers in reply message */
            int size;
            MPI_Get_count(&status, MPI_INT, &size);

            /* receive message(s) and set return code */
            rc = CIRCLE_work_receive(qp, st, source, size);

            /* send request asking for work can be completed now,
             * since we reuse status here, be careful we don't trash
             * it between iprobe and get_count calls */
            MPI_Wait(&st->work_requested_req, &status);

            /* flip flag to indicate we're no longer waiting for a reply */
            st->work_requested = 0;
        }
    }

    return rc;
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

    /* TODO; use isend to avoid deadlock, but in that case, be careful
     * to not overwrite space in queue before sends complete */

    /* send item count, total bytes, and offsets of each item */
    MPI_Ssend(st->request_offsets, numoffsets, \
              MPI_INT, dest, WORK, *st->mpi_state_st->work_comm);

    /* send data */
    char* buf = qp->base + start_offset;
    MPI_Ssend(buf, bytes, MPI_CHAR, \
              dest, WORK, *st->mpi_state_st->work_comm);

    LOG(CIRCLE_LOG_DBG,
        "Sent %d of %d items to %d.", st->request_offsets[0], qp->count, dest);

    /* subtract elements from our queue */
    qp->count -= count;

    return 0;
}

/**
 * Checks for outstanding work requests
 */
int32_t CIRCLE_check_for_requests(CIRCLE_internal_queue_t* qp, CIRCLE_state_st* st)
{
    int i;

    /* get MPI communicator */
    MPI_Comm comm = *st->mpi_state_st->work_comm;

    /* get pointer to array of requests */
    MPI_Request* requests = st->mpi_state_st->request_request;

    /* This loop is only excuted once.  It is used to initiate receives.
     * When a received is completed, we repost it immediately to capture
     * the next request */
    if(! st->request_pending_receive) {
        for(i = 0; i < st->size; i++) {
            if(i != st->rank) {
                st->request_recv_buf[i] = 0;
                MPI_Recv_init(&st->request_recv_buf[i], 1, MPI_INT, i,
                              WORK_REQUEST, comm, &requests[i]);

                MPI_Start(&requests[i]);
            }
        }

        st->request_pending_receive = 1;
    }

    /* record list of requesting ranks in requestors
     * and number in rcount */
    int* requestors = st->mpi_state_st->requestors;
    int rcount = 0;

    /* Test to see if any posted receive has completed */
    for(i = 0; i < st->size; i++) {
        if(i != st->rank) {
            st->request_flag[i] = 0;

            if(MPI_Test(&requests[i], &st->request_flag[i],
                        &st->mpi_state_st->request_status[i]) != MPI_SUCCESS) {
                MPI_Abort(comm, LIBCIRCLE_MPI_ERROR);
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
        int id = requestors[i];
        MPI_Start(&requests[id]);
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

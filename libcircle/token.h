#ifndef TOKEN_H
#define TOKEN_H

#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <mpi.h>

#include "queue.h"
#include "lib.h"

enum tags {
    WHITE,
    BLACK,
    TERMINATE = -1,
    CIRCLE_TAG_WORK_REQUEST,
    CIRCLE_TAG_WORK_REPLY,
    CIRCLE_TAG_WORK_RECEIPT,
    CIRCLE_TAG_TOKEN,
    CIRCLE_TAG_REDUCE,
    CIRCLE_TAG_BARRIER,
    CIRCLE_TAG_TERM,
    CIRCLE_TAG_ABORT_REQUEST,
    CIRCLE_TAG_ABORT_REPLY,
    CIRCLE_TAG_ABORT_REDUCE,
    MSG_VALID,
    MSG_INVALID,
    PAYLOAD_ABORT = -32
};

typedef struct options {
    char* beginning_path;
    int  verbose;
} options;

/* records info about the tree of spawn processes */
typedef struct CIRCLE_tree_state_st {
    int rank;         /* our global rank (0 to ranks-1) */
    int ranks;        /* number of nodes in tree */
    int parent_rank;  /* rank of parent */
    int children;     /* number of children we have */
    int* child_ranks; /* global ranks of our children */
} CIRCLE_tree_state_st;

typedef struct CIRCLE_state_st {
    /* communicator and our rank and its size */
    MPI_Comm comm;
    int rank;
    int size;

    /* tracks state of token */
    int token_is_local;          /* flag indicating whether we have the token */
    int token_proc;              /* current color of process: WHITE, BLACK, TERMINATE */
    int token_buf;               /* buffer holding current token color */
    int token_src;               /* rank of process who will send token to us */
    int token_dest;              /* rank of process to which we send token */
    MPI_Request token_send_req;  /* request associated with pending receive */

    /* offset arrays are used to transfer length of items while sending work */
    int offsets_count;     /* number of offsets in work and request offset arrays */
    int* offsets_recv_buf; /* buffer in which to receive an array of offsets when receiving work */
    int* offsets_send_buf; /* buffer to specify offsets while sending work */

    /* these are used for persistent receives of work request messages
     * from other tasks */
    int* requestors; /* list of ranks requesting work from us */

    /* used to randomly pick next process to requeset work from */
    unsigned seed;      /* seed for random number generator */
    int next_processor; /* rank of next process to request work from */

    /* manage state for requesting work from other procs */
    int work_requested;             /* flag indicating we have requested work */
    int work_requested_rank;        /* rank of process we requested work from */

    /* tree used for collective operations */
    CIRCLE_tree_state_st tree;   /* parent and children of tree */

    /* manage state for reduction operations */
    int reduce_enabled;          /* flag indicating whether reductions are enabled */
    double reduce_time_last;     /* time at which last reduce ran */
    double reduce_time_interval; /* seconds between reductions */
    int reduce_outstanding;      /* flag indicating whether a reduce is outstanding */
    int reduce_replies;          /* keeps count of number of children who have replied */
    long long int reduce_buf[3]; /* local reduction buffer */

    /* manage state for barrier operations */
    int barrier_started; /* flag indicating whether local process has initiated barrier */
    int barrier_up;      /* flag indicating whether we have sent message to parent */
    int barrier_replies; /* keeps count of number of chidren who have replied */

    /* manage state for termination allreduce operations */
    int term_tree_enabled; /* flag indicating whether to use tree-based termination */
    int work_outstanding;  /* counter to track number of outstanding work transfer messages */
    int term_flag;    /* whether we have sent work to anyone since last allreduce */
    int term_up;      /* flag indicating whether we have sent message to parent */
    int term_replies; /* keeps count of number of chidren who have replied */

    /* manage state for abort broadcast tree */
    int abort_state;        /* flag tracking whether process is in abort state or not */
    int abort_outstanding;  /* flag indicating whether we are waiting on abort reply messages */
    int abort_num_req;      /* number of abort requests */
    MPI_Request* abort_req; /* pointer to array of MPI_Requests for abort messages */

    /* profiling counters */
    int32_t local_objects_processed; /* number of locally completed work items */
    uint32_t local_work_requested;   /* number of times a process asked us for work */
    uint32_t local_no_work_received; /* number of times a process asked us for work */
} CIRCLE_state_st;

/* given the rank of the calling process, the number of ranks in the job,
 * and a degree k, compute parent and children of tree rooted at rank 0
 * and store in tree structure */
void CIRCLE_tree_init(int32_t rank, int32_t ranks, int32_t k, MPI_Comm comm, CIRCLE_tree_state_st* t);

/* free resources allocated in CIRCLE_tree_init */
void CIRCLE_tree_free(CIRCLE_tree_state_st* t);

/* initiate and execute reduction in background */
void CIRCLE_reduce_check(CIRCLE_state_st* st, int count, int cleanup);

/* execute synchronous reduction */
void CIRCLE_reduce_sync(CIRCLE_state_st* st, int count);

/* start non-blocking barrier */
void CIRCLE_barrier_start(CIRCLE_state_st* st);

/* test for completion of non-blocking barrier,
 * returns 1 when all procs have called barrier_start (and resets),
 * returns 0 otherwise */
int CIRCLE_barrier_test(CIRCLE_state_st* st);

/* test for abort, forward abort messages on tree if needed,
 * draining incoming abort messages */
void CIRCLE_abort_check(CIRCLE_state_st* st, int cleanup);

/* execute an allreduce to determine whether any rank has entered
 *  * the abort state, and if so, set all ranks to be in abort state */
void CIRCLE_abort_reduce(CIRCLE_state_st* st);

void CIRCLE_get_next_proc(CIRCLE_state_st* st);

/* checks for and receives an incoming token message,
 * then updates state */
void CIRCLE_token_check(CIRCLE_state_st* st);

int  CIRCLE_check_for_term(CIRCLE_state_st* st);

int  CIRCLE_check_for_term_allreduce(CIRCLE_state_st* st);

void CIRCLE_workreceipt_check(CIRCLE_internal_queue_t* queue,
                          CIRCLE_state_st* state);

void CIRCLE_workreq_check(CIRCLE_internal_queue_t* queue,
                          CIRCLE_state_st* state,
                          int cleanup);

int32_t CIRCLE_request_work(CIRCLE_internal_queue_t* queue,
                            CIRCLE_state_st* state,
                            int cleanup);

void CIRCLE_send_no_work(int32_t dest);

int8_t CIRCLE_extend_offsets(CIRCLE_state_st* st, int32_t size);

void CIRCLE_print_offsets(uint32_t* offsets, int32_t count);

void CIRCLE_bcast_abort(void);

#endif /* TOKEN_H */

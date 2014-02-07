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
    DONE,
    TERMINATE = -1,
    WORK_REQUEST,
    WORK,
    CIRCLE_TAG_TOKEN,
    SUCCESS,
    CIRCLE_TAG_REDUCE,
    ABORT = -32
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

typedef struct CIRCLE_mpi_state_st {
    MPI_Status work_status;
    MPI_Status* request_status;

    MPI_Request work_offsets_request;
    MPI_Request work_request;
    MPI_Request* request_request;

    /* records list of ranks requesting work from us,
     * must have one slot for each neighbor */
    int* requestors;

    MPI_Comm* work_comm;

    int hostname_length;
    char hostname[MPI_MAX_PROCESSOR_NAME];
} CIRCLE_mpi_state_st;

typedef struct CIRCLE_state_st {
    CIRCLE_mpi_state_st* mpi_state_st;

    int8_t verbose;

    int32_t rank;
    int32_t size;

    /* these are used for persistent receives of work request messages
     * from other tasks */
    int* request_flag;
    int* request_recv_buf;
    int8_t request_pending_receive; /* indicates whether we have created our peristent requests */

    /* tracks state of token */
    int8_t token_flag;          /* flag indicating whether we have the token */
    int8_t token;               /* current color of process: WHITE, BLACK, TERMINATE */
    MPI_Comm token_comm;        /* communicator for all token traffic */
    int32_t token_partner_recv; /* rank of process who will send token to us */
    int32_t token_partner_send; /* rank of process to which we send token */
    int8_t token_recv_pending;  /* flag indicating whether we have a receive posted for token */
    int8_t token_recv_buf;      /* buffer to receive incoming token */
    MPI_Request token_recv_req; /* request associated with pending receive */

    /* used to randomly pick next process to requeset work from */
    unsigned seed;          /* seed for random number generator */
    int32_t next_processor; /* rank of next process to request work from */

    /* offset arrays are used to transfer length of items while sending work */
    int32_t offsets_count;  /* number of offsets in work and request offset arrays */
    int* offsets_recv_buf;  /* buffer in which to receive an array of offsets when receiving work */
    int* offsets_send_buf;  /* buffer to specify offsets while sending work */

    /* profiling counters */
    int32_t local_objects_processed; /* number of locally completed work items */
    uint32_t local_work_requested;   /* number of times a process asked us for work */
    uint32_t local_no_work_received; /* number of times a process asked us for work */

    /* manage state for abort */
    int abort; /* flag indicating whether we are in an abort state */

    /* manage state for requesting work from other procs */
    int work_requested;             /* flag indicating we have requested work */
    int work_requested_rank;        /* rank of process we requested work from */
    MPI_Request work_requested_req; /* MPI req associated with isend */

    /* manage state for reduction operations */
    CIRCLE_tree_state_st tree;   /* parent and children of reduction tree */
    int reduce_enabled;          /* flag indicating whether reductions are enabled */
    double reduce_time_last;     /* time at which last reduce ran */
    double reduce_time_interval; /* seconds between reductions */
    int reduce_outstanding;      /* flag indicating whether a reduce is outstanding */
    int reduce_replies;          /* keeps count of number of children who have replied */
    int32_t reduce_buf;          /* local reduction buffer */
} CIRCLE_state_st;

/* given the rank of the calling process, the number of ranks in the job,
 * and a degree k, compute parent and children of tree rooted at rank 0
 * and store in tree structure */
void CIRCLE_tree_init(int32_t rank, int32_t ranks, int32_t k, MPI_Comm comm, CIRCLE_tree_state_st* t);

/* free resources allocated in CIRCLE_tree_init */
void CIRCLE_tree_free(CIRCLE_tree_state_st* t);

/* initiate and execute reduction in background */
void CIRCLE_reduce_progress(CIRCLE_state_st* st, int count);

void CIRCLE_get_next_proc(CIRCLE_state_st* st);
void CIRCLE_send_no_work(int32_t dest);
int32_t  CIRCLE_check_for_term(CIRCLE_state_st* st);
void  CIRCLE_wait_on_probe(CIRCLE_state_st* st, int32_t source, int32_t tag, \
                           int* terminate, int* msg, MPI_Status* status);

int32_t  CIRCLE_check_for_requests(CIRCLE_internal_queue_t* queue, \
                                   CIRCLE_state_st* state);
int32_t  CIRCLE_request_work(CIRCLE_internal_queue_t* queue, \
                             CIRCLE_state_st* state);

void CIRCLE_send_work_to_many(CIRCLE_internal_queue_t* queue, \
                              CIRCLE_state_st* state, \
                              int* requestors, int32_t rcount);
int32_t  CIRCLE_send_work(CIRCLE_internal_queue_t* queue, \
                          CIRCLE_state_st* state, \
                          int32_t dest, int32_t count);

int8_t CIRCLE_extend_offsets(CIRCLE_state_st* st, int32_t size);
void CIRCLE_print_offsets(uint32_t* offsets, int32_t count);

void CIRCLE_bcast_abort(void);

#endif /* TOKEN_H */

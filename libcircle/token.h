#ifndef TOKEN_H
#define TOKEN_H

#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <mpi.h>

#include "queue.h"

enum tags {
    WHITE,
    BLACK,
    DONE,
    TERMINATE=-1,
    WORK_REQUEST,
    WORK,
    TOKEN,
    SUCCESS,
    ABORT=-32
};

typedef struct options
{
    char *beginning_path;
    int  verbose;
} options;

typedef struct CIRCLE_mpi_state_st
{
    MPI_Status term_status;
    MPI_Status work_offsets_status;
    MPI_Status work_status;
    MPI_Status *request_status;

    MPI_Request term_request;
    MPI_Request work_offsets_request;
    MPI_Request work_request;
    MPI_Request *request_request;
} CIRCLE_mpi_state_st;

typedef struct CIRCLE_state_st
{
    CIRCLE_mpi_state_st *mpi_state_st;

    int verbose;
    int rank;
    int size;
    int have_token;
    int token;
    int next_processor;
    int token_partner;

    int term_flag;
    int work_flag;
    int *request_flag;
    int work_pending_request;
    int request_pending_receive;
    int term_pending_receive;
    int incoming_token;

    unsigned int *work_offsets;
    unsigned int *request_offsets;

    int *request_recv_buf;
    int work_request_tries;
} CIRCLE_state_st;

void CIRCLE_probe_messages        ( CIRCLE_state_st *st);
void CIRCLE_send_no_work          ( CIRCLE_state_st *st, int dest);
void CIRCLE_cleanup_work_messages ( CIRCLE_state_st *st );
int  CIRCLE_check_for_term        ( CIRCLE_state_st *st );
int  CIRCLE_wait_on_probe         ( CIRCLE_state_st *st, int source, int tag, \
                                    int timeout, int reject_requests, \
                                    int exclude_rank);

int  CIRCLE_check_for_requests ( CIRCLE_queue_t *queue, CIRCLE_state_st *state);
int  CIRCLE_request_work       ( CIRCLE_queue_t *queue, CIRCLE_state_st *state);
void CIRCLE_send_work_to_many  ( CIRCLE_queue_t *queue, CIRCLE_state_st *state,\
                                 int *requestors, int rcount);
int  CIRCLE_send_work          ( CIRCLE_queue_t *queue, CIRCLE_state_st *state,\
                                 int dest, int count );

#endif /* TOKEN_H */

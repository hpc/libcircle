#ifndef PSTAT_H
#define PSTAT_H
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <mpi.h>
#include "queue.h"
//#include "hiredis.h"
//#include "async.h"
FILE * logfd;
#ifdef DEBUG
#ifndef LOG
int LOG(char * logmsg, ...)
{
    va_list args;
    va_start(args,logmsg);
    return fprintf(logfd,logmsg,args);
}
#endif
#else
#define LOG(msg,...)
#endif
enum tags { WHITE=10,BLACK=1,DONE=2,TERMINATE=-1,WORK_REQUEST=4, WORK=0xBEEF, TOKEN=0, SUCCESS=0xDEAD};

typedef struct options
{
    char * beginning_path;
    int verbose;
} options;

typedef struct state_st
{
    int verbose;
    int rank;
    int size;
    int have_token;
    int token;
    int next_processor;
    int token_partner;
    MPI_Status term_status;
    MPI_Status work_offsets_status;
    MPI_Status work_status;
    MPI_Status * request_status;
    MPI_Request term_request;
    MPI_Request work_offsets_request;
    MPI_Request work_request;
    MPI_Request * request_request;
    int term_flag;
    int work_flag;
    int * request_flag;
    int work_pending_request;
    int request_pending_receive;
    int term_pending_receive;
    int incoming_token;
    unsigned int * work_offsets;
    unsigned int * request_offsets;
    int * request_recv_buf;
    int work_request_tries;
} state_st;

//redisAsyncContext *redis_context;
void send_work_to_many( work_queue * qp, state_st * st, int * requestors, int rcount);
void send_no_work( int dest, state_st * st );
int send_work( work_queue * qp, state_st * st, int dest, int count );
void print_offsets(unsigned int * offsets, int count);
void dumpq( work_queue * qp);
void printq( work_queue * qp );
int wait_on_probe(int source, int tag,int timeout, int reject_requests, int exclude_rank,state_st * st);
void cleanup_work_messages(state_st * st);
int parse_args( int argc, char *argv[] , options * opts );

/* Worker function, executed by all ranks */
int worker( options * opts );

int check_for_requests( work_queue * qp , state_st * st);

int request_work( work_queue * qp, state_st * st);

int check_for_term( state_st * st );
#endif

/**
 * @file
 * The library source contains the internal implementation of each API hook.
 */

#include <mpi.h>
#include <stdlib.h>
#include "libcircle.h"
#include "log.h"
#include "lib.h"
#include "worker.h"
#include "token.h"

/** The debug stream for all logging messages. */
FILE* CIRCLE_debug_stream;

/** The current log level of library logging output. */
enum CIRCLE_loglevel CIRCLE_debug_level;

/** The rank value of the current node. */
int  CIRCLE_global_rank;

/** A struct which holds a reference to all input given through the API. */
CIRCLE_input_st CIRCLE_INPUT_ST;

/**
 * Initialize internal state needed by libcircle. This should be called before
 * any other libcircle API call.
 *
 * @param argc the number of arguments passed into the program.
 * @param argv the vector of arguments passed into the program.
 *
 * @return the rank value of the current process.
 */
__inline__ int CIRCLE_init(int argc, char* argv[])
{
    CIRCLE_debug_stream = stdout;
    CIRCLE_debug_level = CIRCLE_LOG_INFO;

    CIRCLE_INPUT_ST.work_comm = (MPI_Comm*) malloc(sizeof(MPI_Comm));
    CIRCLE_INPUT_ST.token_comm = (MPI_Comm*) malloc(sizeof(MPI_Comm));

    MPI_Init(&argc, &argv);

    MPI_Comm_dup(MPI_COMM_WORLD, CIRCLE_INPUT_ST.work_comm);
    MPI_Comm_dup(MPI_COMM_WORLD, CIRCLE_INPUT_ST.token_comm);

    MPI_Comm_rank(*CIRCLE_INPUT_ST.token_comm, &CIRCLE_global_rank);

    CIRCLE_INPUT_ST.queue = CIRCLE_internal_queue_init();

    if(CIRCLE_INPUT_ST.queue == NULL)
        { return -1; }
    else
        { return CIRCLE_global_rank; }
}

__inline__ int CIRCLE_create_queue(int id, CIRCLE_cb start, CIRCLE_cb process)
{

    if(start == NULL) {
        LOG(LOG_DBG, \
            "Start callback is null for this queue. But that's ok...");
    }
    else {
        if(CIRCLE_multi_add_start(id, start) < 0) {
            LOG(LOG_ERR, \
                "Failed to add a new start callback. Queues may be corrupted.");
            return -1;
        }
    }

    if(CIRCLE_multi_add_process(id, process) < 0) {
        LOG(LOG_ERR, \
            "Failed to add a new process callback. Queues may be corrupted.");
        return -1;
    }
}

/**
 * Once you've defined and told libcircle about your callbacks, use this to
 * execute your program.
 */
__inline__ void CIRCLE_begin(void)
{
    CIRCLE_worker();
}

/**
 * Call this function to checkpoint libcircle's distributed queue.  Each rank
 * writes a file called circle<rank>.txt
 */
__inline__ void CIRCLE_checkpoint(void)
{
    _CIRCLE_checkpoint();
}

/**
 * Call this function to read in libcircle restart files.
 */
__inline__ void CIRCLE_read_restarts(void)
{
    _CIRCLE_read_restarts();
}

/**
 * Call this function to have all ranks dump a checkpoint file and exit.
 */
__inline__ void CIRCLE_abort(void)
{
    CIRCLE_bcast_abort();
}

/**
 * After your program has executed, give libcircle a chance to clean up after
 * itself by calling this. This should be called after all libcircle API calls.
 */
__inline__ void CIRCLE_finalize(void)
{
    CIRCLE_debug_stream = NULL;

    MPI_Finalize();
}

/**
 * Set the logging level that libcircle should use.
 *
 * @param level the logging level that libcircle should output.
 */
__inline__ void CIRCLE_enable_logging(enum CIRCLE_loglevel level)
{
    CIRCLE_debug_level = level;
}

/* EOF */

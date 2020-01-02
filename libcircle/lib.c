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
int32_t  CIRCLE_global_rank;

/** if we initialized MPI, remember that we need to finalize it */
static int CIRCLE_must_finalize_mpi;

/** Communicator names **/
char CIRCLE_WORK_COMM_NAME[32] = "Libcircle Work Comm";
char CIRCLE_TOKEN_COMM_NAME[32] = "Libcircle Token Comm";

/** A struct which holds a reference to all input given through the API. */
CIRCLE_input_st CIRCLE_INPUT_ST;

/** Handle to the queue */
extern CIRCLE_handle queue_handle;

CIRCLE_handle* CIRCLE_get_handle()
{
    return &queue_handle;
}

/**
 * Initialize internal state needed by libcircle. This should be called before
 * any other libcircle API call.
 *
 * @param argc the number of arguments passed into the program.
 * @param argv the vector of arguments passed into the program.
 *
 * @return the rank value of the current process.
 */
__inline__ int32_t CIRCLE_init(int argc, char* argv[], int user_options)
{
    CIRCLE_debug_stream = stdout;
    CIRCLE_debug_level = CIRCLE_LOG_INFO;

    /* initialize callback pointers */
    CIRCLE_INPUT_ST.create_cb      = NULL;
    CIRCLE_INPUT_ST.process_cb     = NULL;
    CIRCLE_INPUT_ST.reduce_init_cb = NULL;
    CIRCLE_INPUT_ST.reduce_op_cb   = NULL;
    CIRCLE_INPUT_ST.reduce_fini_cb = NULL;

    /* initialize user reduction buffer */
    CIRCLE_INPUT_ST.reduce_buf      = NULL;
    CIRCLE_INPUT_ST.reduce_buf_size = 0;

    /* initialize reduction period to 0 seconds
     * to disable reductions by default */
    CIRCLE_INPUT_ST.reduce_period = 0;

    /* initialize width of communication tree */
    CIRCLE_INPUT_ST.tree_width = 64;

    CIRCLE_set_options(user_options);

    /* determine whether we need to initialize MPI,
     * and remember if we did so we finalize later */
    CIRCLE_must_finalize_mpi = 0;
    int mpi_initialized;

    if(MPI_Initialized(&mpi_initialized) != MPI_SUCCESS) {
        LOG(CIRCLE_LOG_FATAL, "Unable to initialize MPI.");
        return -1;
    }

    if(! mpi_initialized) {
        /* not already initialized, so intialize MPI now */
        if(MPI_Init(&argc, &argv) != MPI_SUCCESS) {
            LOG(CIRCLE_LOG_FATAL, "Unable to initialize MPI.");
            return -1;
        }

        /* remember that we must finalize later */
        CIRCLE_must_finalize_mpi = 1;
    }

    MPI_Comm_dup(MPI_COMM_WORLD, &CIRCLE_INPUT_ST.comm);
    MPI_Comm_set_name(CIRCLE_INPUT_ST.comm, CIRCLE_WORK_COMM_NAME);
    MPI_Comm_rank(CIRCLE_INPUT_ST.comm, &CIRCLE_global_rank);

    CIRCLE_INPUT_ST.queue = CIRCLE_internal_queue_init();

    if(CIRCLE_INPUT_ST.queue == NULL) {
        return -1;
    }
    else {
        return CIRCLE_global_rank;
    }
}

/**
 * Processing and creating work is done through callbacks. Here's how we tell
 * libcircle about our function which creates an initial workload. This call
 * is optional.
 *
 * @param func the callback to be used in the creation stage.
 */
__inline__ void CIRCLE_cb_create(CIRCLE_cb func)
{
    CIRCLE_INPUT_ST.create_cb = func;
}


/**
 * Change run time flags
 */
void CIRCLE_set_options(int user_options)
{
    CIRCLE_INPUT_ST.options = user_options;
    LOG(CIRCLE_LOG_DBG, "Circle options set: %X", user_options);
}

/**
 * Change the width of the k-ary communication tree.
 */
void CIRCLE_set_tree_width(int width)
{
    CIRCLE_INPUT_ST.tree_width = width;
}

/**
 * Change the number of seconds between consecutive reductions.
 */
void CIRCLE_set_reduce_period(int secs)
{
    CIRCLE_INPUT_ST.reduce_period = secs;
}

/**
 * After you give libcircle a way to create work, you need to tell it how that
 * work should be processed.
 *
 * @param func the callback to be used in the process stage.
 */
__inline__ void CIRCLE_cb_process(CIRCLE_cb func)
{
    if(CIRCLE_INPUT_ST.create_cb == NULL) {
        CIRCLE_INPUT_ST.create_cb = func;
    }

    CIRCLE_INPUT_ST.process_cb = func;
}

/**
 * This function will be invoked on all processes to get initial input
 * data for the reduction.
 *
 * @param func the callback to be used to provide data for reduction.
 */
__inline__ void CIRCLE_cb_reduce_init(CIRCLE_cb_reduce_init_fn func)
{
    CIRCLE_INPUT_ST.reduce_init_cb = func;
}

/**
 * This function will be invoked on processes to execute the reduction
 * tree.
 *
 * @param func the callback to be used to combine data during reduction.
 */
__inline__ void CIRCLE_cb_reduce_op(CIRCLE_cb_reduce_op_fn func)
{
    CIRCLE_INPUT_ST.reduce_op_cb = func;
}

/**
 * This function will be invoked on the root (rank 0) to provide the
 * final result of the reduction.
 *
 * @param func the callback to be provide reduction output on root.
 */
__inline__ void CIRCLE_cb_reduce_fini(CIRCLE_cb_reduce_fini_fn func)
{
    CIRCLE_INPUT_ST.reduce_fini_cb = func;
}

/**
 * Call this function to give libcircle initial reduction data.
 *
 * @param buf pointer to buffer holding reduction data
 * @param size size of buffer in bytes
 */
__inline__ void CIRCLE_reduce(const void* buf, size_t size)
{
    /* free existing buffer memory if we have any */
    CIRCLE_free(&CIRCLE_INPUT_ST.reduce_buf);

    /* allocate memory to copy reduction data */
    if(size > 0) {
        /* allocate memory */
        void* copy = malloc(size);

        if(copy == NULL) {
            LOG(CIRCLE_LOG_FATAL, "Unable to allocate %llu bytes for reduction buffer.",
                (unsigned long long) size);
            /* TODO: bail with fatal error */
            return;
        }

        /* copy data from user buffer */
        memcpy(copy, buf, size);

        /* store buffer on input state */
        CIRCLE_INPUT_ST.reduce_buf      = copy;
        CIRCLE_INPUT_ST.reduce_buf_size = size;
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
    CIRCLE_internal_queue_free(CIRCLE_INPUT_ST.queue);

    /* free buffer holding user reduction data */
    CIRCLE_free(&CIRCLE_INPUT_ST.reduce_buf);

    /* free off MPI resources and shut it down */
    MPI_Comm_free(&CIRCLE_INPUT_ST.comm);

    if(CIRCLE_must_finalize_mpi) {
        /* finalize MPI if we initialized it */
        MPI_Finalize();
    }

    CIRCLE_debug_stream = NULL;
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

/**
 * Returns an elapsed time on the calling processor for benchmarking purposes.
 *
 * @return time in seconds since an arbitrary time in the past.
 */
__inline__ double CIRCLE_wtime(void)
{
    return MPI_Wtime();
}

/* EOF */

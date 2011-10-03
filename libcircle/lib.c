#include <mpi.h>
#ifndef __GNUC__
#define __inline__ inline
#endif
#include "libcircle.h"
#include "log.h"
#include "lib.h"
#include "worker.h"
#include "token.h"

FILE *CIRCLE_debug_stream;
enum CIRCLE_loglevel CIRCLE_debug_level;
int  CIRCLE_global_rank;

CIRCLE_input_st CIRCLE_INPUT_ST;

/*
 * Initialize internal state needed by libcircle. This should be called before
 * any other libcircle API call.
 */
__inline__ int CIRCLE_init(int argc, char *argv[])
{
    CIRCLE_debug_stream = stdout;
    CIRCLE_debug_level = CIRCLE_LOG_FATAL;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &CIRCLE_global_rank);
    CIRCLE_INPUT_ST.queue = CIRCLE_queue_init();
    return CIRCLE_global_rank;
}

/*
 * Processing and creating work is done through callbacks. Here's how we tell
 * libcircle about our function which creates an initial workload. This call
 * is optional.
 */
__inline__ void CIRCLE_cb_create(CIRCLE_cb func)
{
    CIRCLE_INPUT_ST.create_cb = func;
}

/*
 * After you give libcircle a way to create work, you need to tell it how that
 * work should be processed.
 */
__inline__ void CIRCLE_cb_process(CIRCLE_cb func)
{
    if(CIRCLE_INPUT_ST.create_cb == NULL)
    {
        CIRCLE_INPUT_ST.create_cb = func;
    }
    CIRCLE_INPUT_ST.process_cb = func;
}

/*
 * Once you've defined and told libcircle about your callbacks, use this to
 * execute your program.
 */
__inline__ void CIRCLE_begin(void)
{
    CIRCLE_worker();
}
/*
 * Call this function to checkpoint libcircle's distributed queue.  Each rank writes a file called circle<rank>.txt
 */
__inline__ void CIRCLE_checkpoint(void)
{
    _CIRCLE_checkpoint();
}

/*
 * Used to read restart files.
 */
__inline__ void CIRCLE_read_restarts(void)
{
    _CIRCLE_read_restarts();
}
/*
 * Call this function to have all ranks dump a checkpoint file and exit. 
 */
__inline__ void CIRCLE_abort(void)
{
    CIRCLE_bcast_abort();
}

/*
 * After your program has executed, give libcircle a chance to clean up after
 * itself by calling this. This should be called after all libcircle API calls.
 */
__inline__ void CIRCLE_finalize(void)
{
    CIRCLE_debug_stream = NULL;
    MPI_Finalize();
}

/*
 * Set the logging level that libcircle should use.
 */
__inline__ void CIRCLE_enable_logging(enum CIRCLE_loglevel level)
{
    CIRCLE_debug_level = level;
}

/* EOF */

#include <mpi.h>

#include "libcircle.h"
#include "log.h"
#include "lib.h"
#include "worker.h"

FILE *CIRCLE_debug_stream;
int  CIRCLE_debug_level;
int  CIRCLE_global_rank;

CIRCLE_input_st CIRCLE_INPUT_ST;

/*
 * Initialize internal state needed by libcircle. This should be called before
 * any other libcircle API call.
 */
int CIRCLE_init(int argc, char *argv[])
{
    CIRCLE_debug_stream = stderr;
    CIRCLE_debug_level = LIBCIRCLE_LOGLEVEL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &CIRCLE_global_rank);

    return CIRCLE_global_rank;
}

/*
 * Processing and creating work is done through callbacks. Here's how we tell
 * libcircle about our function which creates an initial workload. This call
 * is optional.
 */
void CIRCLE_cb_create(CIRCLE_cb func)
{
    CIRCLE_INPUT_ST.create_cb = func;
}

/*
 * After you give libcircle a way to create work, you need to tell it how that
 * work should be processed.
 */
void CIRCLE_cb_process(CIRCLE_cb func)
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
void CIRCLE_begin(void)
{
    CIRCLE_worker();
}

/*
 * Call this function to have all ranks dump a checkpoint file and exit. 
 */
void CIRCLE_abort(void)
{
    MPI_Abort(MPI_COMM_WORLD,LIBCIRCLE_MPI_ERROR);
}

/*
 * After your program has executed, give libcircle a chance to clean up after
 * itself by calling this. This should be called after all libcircle API calls.
 */
void CIRCLE_finalize(void)
{
    CIRCLE_debug_stream = NULL;

    MPI_Finalize();
}

/* EOF */

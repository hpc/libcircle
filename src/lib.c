#include <mpi.h>

#include "libcircle.h"
#include "log.h"
#include "lib.h"
#include "worker.h"

FILE *dbgstream;
int  debug_level;
int global_rank;
CIRCLE_input_st CIRCLE_INPUT_ST;

/*
 * Initialize internal state needed by libcircle. This should be called before
 * any other libcircle API call.
 */
void CIRCLE_init(int argc, char *argv[])
{
    dbgstream = stderr;
    debug_level = LIBCIRCLE_LOGLEVEL;

    MPI_Init(&argc,&argv);
}

/*
 * Processing and creating work is done through callbacks. Here's how we tell
 * libcircle about our function which creates work.
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
 * After your program has executed, give libcircle a chance to clean up after
 * itself by calling this. This should be called after all libcircle API calls.
 */
void CIRCLE_finalize(void)
{
    fflush(dbgstream);
    dbgstream = NULL;

    MPI_Finalize();
}

/* EOF */

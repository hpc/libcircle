#include "libcircle.h"
#include "log.h"

FILE *dbgstream;
int  debug_level;

/*
 * Initialize internal state needed by libcircle. This should be called before
 * any other libcircle API call.
 */
void CIRCLE_init(int argc, char *argv[])
{
    dbgstream = stderr;
    debug_level = LOG_DBG;

    MPI_Init(&argc,&argv);

    return 0;
}

/*
 * Processing and creating work is done through callbacks. Here's how we tell
 * libcircle about our function which creates work.
 */
void CIRCLE_cb_create(CIRCLE_cb func)
{
    /* FIXME */
}

/*
 * After you give libcircle a way to create work, you need to tell it how that
 * work should be processed.
 */
void CIRCLE_cb_process(CIRCLE_cb func)
{
    /* FIXME */
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

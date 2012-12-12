#include "mpii.h"

static MPI_Context ncontext = 0;
static Mutex ncontext_mutex;

PRIVATE void MPII_Context_init ()
{
    new_mutex (ncontext_mutex);
}

PRIVATE MPI_Context MPII_New_context ()
{
    int result;
    lock (ncontext_mutex);
    result = ncontext++;
    unlock (ncontext_mutex);
    return result;
}

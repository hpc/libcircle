#define DEFINE_GTHREAD_GLOBALS
#include "mpii.h"

/* Options */
/* Number of threads (MPI processes to use).  Default to just one. */
PRIVATE int MPII_nthread = 1;

/* Stack size of each spawned thread.  0 indicates the default (typically one
 * megabyte).  Otherwise measured in bytes.  Set to as low as possible!  Not
 * supported on all systems (e.g., POSIX threads).
 */
PRIVATE int MPII_Stack_size_val = 0;
PUBLIC int *MPII_Stack_size_ptr (void)
{
    return &MPII_Stack_size_val;
}

/* Number of processors to use.  This corresponds to the concurrency level
 * in Solaris threads, and has no equivalent for POSIX threads.
 */
PRIVATE int MPII_Num_proc_val = 1;
PUBLIC int *MPII_Num_proc_ptr (void)
{
    return &MPII_Num_proc_val;
}

/* Global keys */
PRIVATE Key MPII_commworld_key;
PRIVATE Key MPII_me_key;

/* Other globals */
PRIVATE MPI_Request MPII_Request_null_val
  = {0, 0, MPII_REQUEST_NULL, NULL, NULL, -1, -1, -1, -2};
PUBLIC MPI_Request *MPII_Request_null_ptr (void)
{
    return &MPII_Request_null_val;
}

/* Global information about the various running threads */
PRIVATE MPI_Comm *MPII_worlds = NULL;
PRIVATE Thread *MPII_threads = NULL;

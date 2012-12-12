#define THREAD_AWARE
/* This version has problems with being used before initialization since it
 * calls get_tsd which it isn't allowed to.  Can't just fix get_tsd because
 * need to catch before then--initial thread's "id" may change.
 */

#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include "mpii.h" /* really only gthread.h and optionally profile.h */

static Mutex mutex;
static int initialized = 0;

/* Someone is required to call this (at worst) immediately before you spawn
 * any threads.  You can call this multiple times from the same thread, e.g.
 * you can call it any number of times before any threads are spawned.
 */
void MPII_Get_global_init (void)
{
  if (!initialized)
  {
    new_mutex (mutex);
    initialized++;
  }
}

/* You can *only* call this when either:
 *    - you have called MPII_Get_global_init() at least once
 *    - there is only one thread that could possibly call it at once
 */
void *MPII_Get_global (Key **key, int size, void *init)
{
  void *ptr;
  
  if (*key == (Key *) 0)
  {
    int mpi_init;
    MPI_Initialized (&mpi_init);
    if (mpi_init)
    {
      if (!initialized)
      {
        new_mutex (mutex);
        initialized++;
      }
      lock (mutex);
    }
    if (*key == (Key *) 0) /* still */
    {
      *key = (Key *) malloc (sizeof (Key));
      new_tsd (**key);
    }
    if (mpi_init)
      unlock (mutex);
  }

  ptr = get_tsd (**key);
  if (ptr == (void *) 0)
  {
    ptr = (void *) malloc (size);
    if (ptr == (void *) 0)
    {
      perror ("g2tsd: MPII_Get_global: malloc failed");
      exit (1);
    }
    memcpy (ptr, init, size);
    set_tsd (**key, ptr);
  }
  
  return ptr;
}

#define THREAD_AWARE

#include <errno.h>
//#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include "mpii.h" /* really only gthread.h and optionally profile.h */

static Mutex mutex;
static int initialized = 0;
static Thread first_thread;

/* Someone is required to call this (at worst) immediately before you spawn
 * any threads.  It must be called by the "first" thread first.  You can call
 * this multiple times in serial, but never concurrently.
 */
void MPII_Get_global_init (void)
{
  if (!initialized)
  {
    new_mutex (mutex);
    first_thread = thread_id ();
    initialized++;
  }
}

/* Sets this to be the new "first thread."  Also initializes this library. */
void MPII_Get_global_init_first (void)
{
  MPII_Get_global_init ();  /* Do not remove this.  It's exploited in init.c */
  first_thread = thread_id ();
}

typedef struct
{
  Key *key;      /* Pointer to the key, just like usual.
                  * Used by everyone except the first thread.
                  */
  void *oneval;  /* Value for the first thread; important for accessing a TSD
                  * before anything's initialized (e.g., thread system).  We
                  * really have to catch this here, so that "first thread" can
                  * be reassigned.
                  */
} KeyOne;

/* You can *only* call this when either:
 *    - you have called MPII_Get_global_init() at least once
 *    - there is only one thread that could possibly call it at once
 */
void *MPII_Get_global (KeyOne *key, int size, void *init)
{
  void *ptr;

  if (key->key == (Key *) 0)
  {
    MPII_Get_global_init ();  /* We need first_thread, and later mutex */

    if (thread_id () == first_thread)
    {
      if (key->oneval == (void *) 0)
      {
        key->oneval = (void *) malloc (size);
        if (key->oneval == (void *) 0)
        {
          perror ("g2tsd: MPII_Get_global: malloc failed");
          exit (1);
        }
        memcpy (key->oneval, init, size);
      }
      return key->oneval;
    }

    lock (mutex);
    if (key->key == (Key *) 0) /* still */
    {
      key->key = (Key *) malloc (sizeof (Key));
      new_tsd (*(key->key));
    }
    unlock (mutex);
  }

  ptr = get_tsd (*(key->key));
  if (ptr == (void *) 0)
  {
    ptr = (void *) malloc (size);
    if (ptr == (void *) 0)
    {
      perror ("g2tsd: MPII_Get_global: malloc failed");
      exit (1);
    }
    memcpy (ptr, init, size);
    set_tsd (*(key->key), ptr);
  }
  
  return ptr;
}

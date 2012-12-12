/* Custom thread-specific data maintainance.  Only used if MPII_TSD is defined
 * (presumably in gthread.h).  It can be defined to one of the following
 * constants (defined in gthread.h):
 *
 * MPII_TSD_ALMOST_INTERVAL: thread ids are an interval of integers except for
 *     the first value, and tid1 < tid2 iff thread1 was spawned before thread2
 *     (except perhaps for the first value).
 * MPII_TSD_SINGLE: there already exists a single thread-specific data, defined
 *     by get_tsd1 and set_tsd1 (presumably in gthread.h).
 */

#include <stdio.h>
#include "mpii.h"

#ifdef MPII_TSD

static int initialized = 0;

/****************************************************************************/

#if MPII_TSD == MPII_TSD_ALMOST_INTERVAL

/* Stores the tid of the first thread.  It is handled specially because it
 * usually doesn't relate to other tids.
 */
static int first_tid = -1;

/* Stores the tid of the second thread.  Hence, an index is computed by
 * tid-second_tid+1, unless tid == first_tid.
 */
static int second_tid = -1;

#define map(tid) ((tid) == first_tid ? 0 : (tid)-second_tid+1)

/* Stores the id of the last thread that registered. */
static int last_tid = -1;

/* The total number of threads that will register. */
static int nthread = -1;

/* Call this initially, and then call MPII_Tsd_thread once for each thread.
 * n must accurately reflect the number of threads to appear.
 * This must be called before new_tsd, but Tsd_thread need not be called before
 * new_tsd.  Tsd_thread must be called on the local thread and all previous
 * threads before you can call set_tsd on the local thread.
 */
PRIVATE void MPII_Tsd_init (int n)
{
  nthread = n;
  initialized = 1;
}

/* Called by each spawned thread.  These must be done in order.  See the
 * documentation of MPII_Tsd_init above for more details on restrictions.
 */
PRIVATE void MPII_Tsd_thread (Thread id)
{
  if (first_tid == -1)
    first_tid = last_tid = id;
  else if (first_tid == last_tid)
    second_tid = last_tid = id;
  else if (id == last_tid + 1)
  {
    last_tid++;
    if (last_tid - second_tid + 2 > nthread)
    {
      fprintf (stderr, "MPII_Tsd: Extra threads were spawned!\n");
      exit (1);
    }
  }
  else
  {
    fprintf (stderr, "MPII_Tsd: Your thread system doesn't obey the expected property that thread ids\n");
    fprintf (stderr, "MPII_Tsd: form an interval of integers, and tid1 < tid2 iff thread1 was spawned\n");
    fprintf (stderr, "MPII_Tsd: before thread2, except perhaps for the very first thread id.  You can\n");
    fprintf (stderr, "MPII_Tsd: try commenting out MPII_TSD in include/gthread.h and recompiling\n");
    fprintf (stderr, "MPII_Tsd: TOMPI (make clean; make).  This will use the thread-specific data\n");
    fprintf (stderr, "MPII_Tsd: facility in the thread system.  However, this alternative was likely\n");
    fprintf (stderr, "MPII_Tsd: chosen to avoid bugs in that facility.\n");
    fprintf (stderr, "MPII_Tsd: TIDs: first = %d, second = %d, last = %d, next = %d\n", first_tid, second_tid, last_tid, id);
    exit (1);
  }
}

PRIVATE int MPII_New_tsd (Key *key)
{
  int i;
  
  if (!initialized)
    fprintf (stderr, "MPII_Tsd: Attempt to create thread-specific data before calling MPI_Init\n");

  *key = (void **) malloc (sizeof (void *) * nthread);
  if (*key == NULL)
  {
    perror ("MPII_Tsd: Couldn't create thread-specific data");
    exit (1);
  }

  for (i = 0; i < nthread; i++)
    (*key)[i] = NULL;

  return 0;
}

PRIVATE void *MPII_Get_tsd (Key *key)
{
  int id = thread_id ();
#if 0
  printf ("getting %p[%d->%d]...\n", *key, id, map(id));
  printf ("got %p[%d->%d] as %p\n", *key, id, map(id), (*key)[map(id)]);
#endif
  return (*key)[map(id)];
}

PRIVATE int MPII_Set_tsd (Key *key, void *val)
{
  int id = thread_id ();
#if 0
  printf ("setting %p[%d->%d] to %p\n", *key, id, map(id), val);
#endif
  (*key)[map(id)] = val;
  return 0;
}

/****************************************************************************/

#elif MPII_TSD == MPII_TSD_SINGLE

#define BLOCK_SIZE 8     /* Small to allow a large number of threads;
                            approx. 64 bytes per thread. */
static int nkey = 0;
static Mutex mutex;

typedef struct
{
    void **vals;
    int n;
} Tsd1;

static Tsd1 *temp_tsd1 = 0;

void MPII_Tsd_init (int n)
{
  new_mutex (mutex);
  initialized = 1;
  set_tsd1 (temp_tsd1);
}

void MPII_Tsd_thread (Thread id)
{
}

int MPII_New_tsd (Key *key)
{
  if (initialized)
    lock (mutex);
  *key = nkey++;
  if (initialized)
    unlock (mutex);
  /*printf ("created key %d\n", *key);*/

  return 0;
}

#define find_tsd \
    int i = *key; \
    Tsd1 *tsd = (initialized ? (Tsd1 *) get_tsd1 () : temp_tsd1); \
    if (tsd == NULL) \
    { \
        tsd = (Tsd1 *) malloc (sizeof (Tsd1)); \
        if (tsd == NULL) \
        { \
            perror ("MPII_Tsd: Couldn't initialize thread-specific data 1"); \
            exit (1); \
        } \
        tsd->vals = (void **) malloc (BLOCK_SIZE * sizeof (void *)); \
        tsd->n = BLOCK_SIZE; \
        if (initialized) \
          set_tsd1 (tsd); \
        else \
          temp_tsd1 = tsd; \
    } \
    if (i >= tsd->n) \
    { \
        int j, n = i + BLOCK_SIZE - i % BLOCK_SIZE; \
        tsd->vals = (void **) realloc (tsd->vals, n * sizeof (void *)); \
        for (j = tsd->n; j < n; j++) \
            tsd->vals[j] = NULL; \
        tsd->n = n; \
    }

void *MPII_Get_tsd (Key *key)
{
    find_tsd
      /*printf ("getting key %d as %p\n", i, tsd->vals[i]);*/
    return tsd->vals[i];
}

int MPII_Set_tsd (Key *key, void *val)
{
    find_tsd
      /*printf ("setting key %d to %p\n", i, val);*/
    tsd->vals[i] = val;
    return 0;
}

#endif /* various cases of MPII_TSD */
#endif /* defined(MPII_TSD) */

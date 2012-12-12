/* Waiter thread */

#include "mpii.h"

#ifdef MPII_WAITER_THREAD

#undef main
int main (int argc, char *argv[])
{
  int i;
  void *rval;

  /* Start up other threads */
  MPII_Master (&argc, &argv, 0);

  /* Wait for them to exit by joining with them all */
  for (i = 1; i < MPII_nthread; i++)
    join_thread (MPII_threads[i], rval);
  join_thread (MPII_threads[0], rval);

  /* Return the value returned from the first thread (as if it were us) */
  return (long int) rval;
}

#endif

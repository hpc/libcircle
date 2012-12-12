#include "mpii.h"

#include <sys/time.h>
#ifdef TIMEOFDAY
# include <sys/timers.h>
#endif

PUBLIC double MPI_Wtime (void)
{
# if defined(MPII_WTIME)
    extern double MPII_Wtime ();
    return MPII_Wtime ();
# else
#   if defined(TIMEOFDAY)
      struct timespec tp;
      getclock (TIMEOFDAY, &tp);
      return tp.tv_sec + (tp.tv_nsec / 1000000000.0);
#   else
      struct timeval tp;
      gettimeofday (&tp, NULL);
      return tp.tv_sec + (tp.tv_usec / ((double) 1000000.0));
#   endif
# endif
}


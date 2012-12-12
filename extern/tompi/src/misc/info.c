/* Various informational routines */

#include "mpii.h"

PRIVATE void MPII_Thread_info ()
{
  printf ("TOMPI: Using thread system \"%s\"\n", GTHREAD_SYSTEM_NAME);
#ifdef MPII_WTIME
  printf ("TOMPI: Using accurate timer provided by thread system\n");
#endif
}

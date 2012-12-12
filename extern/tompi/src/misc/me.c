#include "mpii.h"

/* Returns the thread's "name" as a MPII_Member structure (actually, a pointer
 * to such a structure).
 */
PRIVATE MPII_Member *MPII_Me (MPI_Comm comm)
{
   MPII_Member *me = (MPII_Member *) get_tsd (MPII_me_key);
   if (me == NULL)
      MPII_Error (comm, MPII_NOT_INITIALIZED);
   return me;
}


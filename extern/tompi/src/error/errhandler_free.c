#include "mpii.h"

void MPII_Errhandler_free (MPI_Errhandler *errhandler)
{
  *errhandler = MPI_ERRHANDLER_NULL;
}

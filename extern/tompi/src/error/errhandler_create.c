#include "mpii.h"

PUBLIC int MPI_Errhandler_create (MPI_Handler_function *function, MPI_Errhandler *errhandler)
{
  *errhandler = function;
  return MPI_SUCCESS;
}


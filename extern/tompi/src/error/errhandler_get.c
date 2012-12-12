#include "mpii.h"

PUBLIC int MPI_Errhandler_get (MPI_Comm comm, MPI_Errhandler *errhandler)
{
  check_comm (comm);
  *errhandler = comm->errhandler;
  return MPI_SUCCESS;
}


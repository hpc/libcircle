#include "mpii.h"

PUBLIC int MPI_Errhandler_set (MPI_Comm comm, MPI_Errhandler errhandler)
{
  check_comm (comm);
  comm->errhandler = errhandler;
  return MPI_SUCCESS;
}


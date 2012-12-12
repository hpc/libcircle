#include "mpii.h"

PUBLIC int MPI_Comm_dup (MPI_Comm comm, MPI_Comm *newcomm)
{
  check_comm (comm);

  *newcomm = mymalloc (1, MPII_Comm);
  (*newcomm)->group = comm->group;
  comm->group->refcnt++;
  (*newcomm)->errhandler = comm->errhandler;
  if (MPII_Is_captain (comm->group))
    (*newcomm)->context = MPII_New_context ();
  MPI_Bcast (&((*newcomm)->context), 1, MPII_CONTEXT_TYPE, 0, comm);
  return 0;
}

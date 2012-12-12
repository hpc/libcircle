#include "mpii.h"

PUBLIC int MPI_Comm_compare (MPI_Comm comm1, MPI_Comm comm2, int *result)
{
  if (comm1 == comm2)
    *result = MPI_IDENT;
  else
  {
    MPI_Group_compare (comm1->group, comm2->group, result);
    if (*result == MPI_IDENT)
      *result = MPI_CONGRUENT;
  }
  return MPI_SUCCESS;
}

#include "mpii.h"

PUBLIC int MPI_Group_rank (MPI_Group group, int *rank)
{
  check_group (group, NULL);
  *rank = group->rank;
  return MPI_SUCCESS;
}

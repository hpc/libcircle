#include "mpii.h"

PUBLIC int MPI_Group_size (MPI_Group group, int *size)
{
  check_group (group, NULL);
  *size = group->size;
  return MPI_SUCCESS;
}

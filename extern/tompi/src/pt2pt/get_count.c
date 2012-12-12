#include "mpii.h"

PUBLIC int MPI_Get_count (MPI_Status *status, MPI_Datatype datatype, int *count)
{
  check_datatype (datatype, NULL);
  if (status->MPII_COUNT % MPII_types[datatype].size == 0)
    *count = status->MPII_COUNT / MPII_types[datatype].size;
  else
    *count = MPI_UNDEFINED;
  return MPI_SUCCESS;
}


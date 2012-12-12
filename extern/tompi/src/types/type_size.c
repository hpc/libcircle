#include "mpii.h"

PUBLIC int MPI_Type_size (MPI_Datatype datatype, int *size)
{
  check_datatype (datatype, NULL);
  *size = MPII_types[datatype].size;
  return MPI_SUCCESS;
}

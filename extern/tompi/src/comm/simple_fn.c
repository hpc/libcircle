#include "mpii.h"

PUBLIC int MPI_NULL_FN ()
{
  return MPI_SUCCESS;
}

PUBLIC int MPI_DUP_FN (MPI_Comm *oldcomm, int *keyval, void *extra_state, void *attribute_val_in, void **attribute_val_out, int *flag)
{
  *flag = 1;
  *attribute_val_out = attribute_val_in;
  return MPI_SUCCESS;
}

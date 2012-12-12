#include "mpii.h"

PRIVATE MPII_Op *MPII_ops = NULL;
PRIVATE int MPII_nops = 0;

PRIVATE void MPII_Ops_init ()
{
  int i;
  MPII_nops = MPII_NUM_BUILTIN_OPS;
  MPII_ops = mymalloc (MPII_NUM_BUILTIN_OPS, MPII_Op);
  for (i = 0; i < MPII_NUM_BUILTIN_OPS; i++)
    MPII_ops[i].commute = 1;
  MPII_ops[MPI_SUM].f = MPII_Sum;
}

PRIVATE void MPII_Sum (void *invec, void *inoutvec, int *len, MPI_Datatype *datatype)
{
  int i;
  switch (*datatype)
  {
    case MPI_INT:
      for (i = 0; i < *len; i++)
        ((int *) inoutvec)[i] += ((int *) invec)[i];
      break;
    case MPI_LONG:
      for (i = 0; i < *len; i++)
        ((long int *) inoutvec)[i] += ((long int *) invec)[i];
      break;
    case MPI_SHORT:
      for (i = 0; i < *len; i++)
        ((short int *) inoutvec)[i] += ((short int *) invec)[i];
      break;
    case MPI_UNSIGNED_SHORT:
      for (i = 0; i < *len; i++)
        ((unsigned short int *) inoutvec)[i] += ((unsigned short int *) invec)[i];
      break;
    case MPI_UNSIGNED:
      for (i = 0; i < *len; i++)
        ((unsigned int *) inoutvec)[i] += ((unsigned int *) invec)[i];
      break;
    case MPI_UNSIGNED_LONG:
      for (i = 0; i < *len; i++)
        ((unsigned long int *) inoutvec)[i] += ((unsigned long int *) invec)[i];
      break;
    case MPI_FLOAT:
      for (i = 0; i < *len; i++)
        ((float *) inoutvec)[i] += ((float *) invec)[i];
      break;
    case MPI_DOUBLE:
      for (i = 0; i < *len; i++)
        ((double *) inoutvec)[i] += ((double *) invec)[i];
      break;
    case MPI_LONG_DOUBLE:
      for (i = 0; i < *len; i++)
        ((long double *) inoutvec)[i] += ((long double *) invec)[i];
      break;
    default:
      MPII_Error (NULL, MPII_BAD_TYPE_FOR_OP);
  }
}

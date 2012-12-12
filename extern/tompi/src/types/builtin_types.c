#include "mpii.h"

PRIVATE MPII_Datatype *MPII_types = NULL;
PRIVATE int MPII_ntypes = 0;

PRIVATE void MPII_Types_init ()
{
  MPII_ntypes = MPII_NUM_BUILTIN_TYPES;
  MPII_types = mymalloc (MPII_NUM_BUILTIN_TYPES, MPII_Datatype);
  MPII_types[MPI_CHAR].size = sizeof (MPII_SIGNED char);
  MPII_types[MPI_SHORT].size = sizeof (MPII_SIGNED short int);
  MPII_types[MPI_INT].size = sizeof (MPII_SIGNED int);
  MPII_types[MPI_LONG].size = sizeof (MPII_SIGNED long int);
  MPII_types[MPI_LONG_LONG].size = sizeof (MPII_SIGNED long long int);
  MPII_types[MPI_UNSIGNED].size = sizeof (unsigned int);
  MPII_types[MPI_UNSIGNED_CHAR].size = sizeof (unsigned char);
  MPII_types[MPI_UNSIGNED_SHORT].size = sizeof (unsigned short int);
  MPII_types[MPI_UNSIGNED_LONG].size = sizeof (unsigned long int);
  MPII_types[MPI_UNSIGNED_LONG_LONG].size = sizeof (unsigned long long int);
  MPII_types[MPI_FLOAT].size = sizeof (float);
  MPII_types[MPI_DOUBLE].size = sizeof (double);
  MPII_types[MPI_LONG_DOUBLE].size = sizeof (long double);
  MPII_types[MPI_BYTE].size = 1;
  MPII_types[MPI_PACKED].size = 1;
}

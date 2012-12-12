#include "mpii.h"

/* Fragile */
#define RETURN(val) { *result = val; return MPI_SUCCESS; }

PUBLIC int MPI_Group_compare (MPI_Group group1, MPI_Group group2, int *result)
{
  int i, j, size;

  check_group (group1, NULL);
  check_group (group2, NULL);

  size = group1->size;
  if (size != group2->size)
    RETURN (MPI_UNEQUAL);

  /* Identical case */
  for (i = 0; i < size; i++)
    if (group1->members[i] != group2->members[i])
      break;
  if (i >= size)
    RETURN (MPI_IDENT);

  for (i = 0; i < size; i++)
  {
    for (j = 0; j < size; j++)
      if (group1->members[i] == group2->members[j])
        break;
    if (j >= size)
      return MPI_UNEQUAL;
  }
  /* If we make it through to here, every member of group1 has found a
   * corresponding member in group2.  We already caught the identical case,
   * so the groups must be similar.
   */
  return MPI_SIMILAR;
}

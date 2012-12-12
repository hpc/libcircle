#include "mpii.h"

PUBLIC int MPI_Group_translate_ranks (MPI_Group group1, int n, int *ranks1, MPI_Group group2, int *ranks2)
{
  int i, j;

  check_group (group1, NULL);
  check_group (group2, NULL);

  for (i = 0; i < n; i++)
  {
    void *desired = group1->members[ranks1[i]];
    ranks2[i] = MPI_UNDEFINED;
    for (j = 0; j < group2->size; j++)
      if (desired == group2->members[j])
      {
        ranks2[i] = j;
        break;
      }
  }
  return 0;
}

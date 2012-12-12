#include "mpii.h"

/* The "captain" of a group is the member of first (zero) rank. */

/* Returns whether the calling process is the caption of the given group. */
PRIVATE int MPII_Is_captain (MPI_Group group)
{
    int rank;
    MPI_Group_rank (group, &rank);
    return (rank == 0);
}

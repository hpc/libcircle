#include "mpii.h"

PUBLIC int MPI_Comm_rank (MPI_Comm comm, int *rank)
{
    check_comm (comm);

    *rank = comm->group->rank;
    return MPI_SUCCESS;
}


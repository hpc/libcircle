#include "mpii.h"

PUBLIC int MPI_Comm_size (MPI_Comm comm, int *size)
{
    check_comm (comm);

    *size = comm->group->size;
    return MPI_SUCCESS;
}


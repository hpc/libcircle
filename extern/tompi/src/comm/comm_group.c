#include "mpii.h"

PUBLIC int MPI_Comm_group (MPI_Comm comm, MPI_Group *group)
{
    check_comm (comm);

    *group = comm->group;
    return MPI_SUCCESS;
}

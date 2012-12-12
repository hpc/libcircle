#include "mpii.h"

PUBLIC int MPI_Bcast (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
   check_comm (comm);
   check_coll_root (root, comm);
   check_datatype (datatype, comm);

   if (comm->group->size == 1)
      return MPI_SUCCESS;

   if (comm->group->rank == root)
   {
      int i, j, rval;
      MPI_Request *reqs = mymalloc (comm->group->size - 1, MPI_Request);

      for (i = 0; i < root; i++)
         if ((rval = MPI_Isend (buffer, count, datatype, i, MPII_BCAST_TAG,
               comm, &(reqs[i]))))
         {
            free (reqs);
            return rval;
         }
      for (j = i++; i < comm->group->size; i++, j++)
         if ((rval = MPI_Isend (buffer, count, datatype, i, MPII_BCAST_TAG,
               comm, &(reqs[j]))))
         {
            free (reqs);
            return rval;
         }

      rval = MPI_Waitall (comm->group->size - 1, reqs, NULL);
      free (reqs);
      return rval;
   }
   else
      return MPI_Recv (buffer, count, datatype, root, MPII_BCAST_TAG, comm,
            NULL);
}


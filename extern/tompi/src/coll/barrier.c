#include "mpii.h"

PUBLIC int MPI_Barrier (MPI_Comm comm)
{
   int rval;

   check_comm (comm);

   if (comm->group->size == 1)
      return MPI_SUCCESS;

   if (comm->group->rank == comm->group->size - 1)
   {
      int i;
      MPI_Request *reqs = mymalloc (comm->group->size - 1, MPI_Request);

      for (i = 0; i < comm->group->rank; i++)
         if ((rval = MPI_Isend (NULL, 0, MPI_BYTE, i, MPII_BARRIER_TAG1,
               comm, &(reqs[i]))))
         {
            free (reqs);
            return rval;
         }
      rval = MPI_Waitall (comm->group->size - 1, reqs, NULL);
      free (reqs);
      if (rval)
         return rval;

      for (i = 0; i < comm->group->rank; i++)
         if ((rval = MPI_Recv (NULL, 0, MPI_BYTE, MPI_ANY_SOURCE,
               MPII_BARRIER_TAG2, comm, NULL)))
            return rval;
   }
   else
   {
      if ((rval = MPI_Recv (NULL, 0, MPI_BYTE, comm->group->size - 1,
            MPII_BARRIER_TAG1, comm, NULL)))
         return rval;
      if ((rval = MPI_Send (NULL, 0, MPI_BYTE, comm->group->size - 1,
            MPII_BARRIER_TAG2, comm)))
         return rval;
   }
   return MPI_SUCCESS;
}


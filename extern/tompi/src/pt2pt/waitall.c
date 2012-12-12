#include "mpii.h"

PUBLIC int MPI_Waitall (int count, MPI_Request *requests, MPI_Status *statuses)
{
   int i, rval;

   for (i = 0; i < count; i++)
      if ((rval = MPI_Wait (&(requests[i]),
            (statuses == NULL ? NULL : &(statuses[i])))))
         return rval;

   return MPI_SUCCESS;
}


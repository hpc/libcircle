#include "mpii.h"

PUBLIC int MPI_Irecv (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
   int rval;

   if ((rval = MPI_Recv_init (buf, count, datatype, source, tag, comm, request)))
      return rval;
   request->persistent = 0;
   /* return MPI_Start (request); */
   return MPI_SUCCESS;
}


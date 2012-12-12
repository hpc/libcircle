#include "mpii.h"

PUBLIC int MPI_Issend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
   int rval;

   if ((rval = MPI_Ssend_init (buf, count, datatype, dest, tag, comm, request)))
      return rval;
   request->persistent = 0;
   return MPI_Start (request);
}


#include "mpii.h"

PUBLIC int MPI_Recv_init (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
   check_comm (comm);
   check_source_rank (source, comm);
   check_datatype (datatype, comm);

   request->active = 0;
   request->persistent = 1;
   request->type = MPII_REQUEST_RECV;
   request->comm = comm;
   request->buf = buf;
   request->count = count;
   request->datatype = datatype;
   request->srcdest = source;
   request->tag = tag;

   return MPI_SUCCESS;
}


#include "mpii.h"

PUBLIC int MPI_Recv (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
   MPI_Request req;
   MPII_Msg msg;
   MPII_Member *member;
   int rval, retry = 0;

   check_comm (comm);
   check_datatype (datatype, comm);
   if (source < 0 || source > comm->group->size){
     if (source == MPI_PROC_NULL)
     {
       /* The following two equalities are defined in MPI-1 (section 3.11) */
       status->MPI_SOURCE = MPI_PROC_NULL;
       status->MPI_TAG = MPI_ANY_TAG;
       status->MPII_COUNT = 0;
       return MPI_SUCCESS;
     }
     else if (source != MPI_ANY_SOURCE){
       return MPII_Error (comm, MPII_RANK_RANGE);
     }
   }

   req.type = MPII_REQUEST_RECV;
   req.comm = comm;
   req.buf = buf;
   req.count = count;
   req.datatype = datatype;
   req.srcdest = source;
   req.tag = tag;

   post_recv ();

   member = MPII_Me (comm);
   lock (member->mutex);
      while (!MPII_queue_search (&retry, &(member->queue), (void *)MPII_match_recv,
                                 &req, &msg))
         wait (member->cond, member->mutex);
   unlock (member->mutex);

   if (status != NULL)
   {
      status->MPI_SOURCE = msg.req->comm->group->rank;
      status->MPI_TAG = msg.req->tag;
      status->MPII_COUNT = msg.req->count * MPII_types[msg.req->datatype].size;
   }

   if (datatype != msg.req->datatype)
      rval = MPII_Error (comm, MPII_TYPE_MISMATCH);
   else if (count >= msg.req->count)
   {
      memcpy (buf, msg.req->buf, msg.req->count * MPII_types[datatype].size);
      rval = MPI_SUCCESS;
   }
   else
   {
      memcpy (buf, msg.req->buf, count * MPII_types[datatype].size);
      rval = MPII_Error (comm, MPII_OVERFLOW);
   }

   notify_sender (((MPII_Member **) msg.req->comm->group->members)
         [msg.req->comm->group->rank], msg, member);

   return rval;
}


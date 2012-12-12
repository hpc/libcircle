#include "mpii.h"

PUBLIC int MPI_Ssend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
   MPI_Request req;
   MPII_Msg msg;
   MPII_Member *member;
   int retry = 0;

   check_comm (comm);
   check_datatype (datatype, comm);
   if (dest < 0 || dest > comm->group->size){
       if (dest == MPI_PROC_NULL){
           return MPI_SUCCESS;
       }
       else{
           return MPII_Error (comm, MPII_RANK_RANGE);
       }
   }

   req.type = MPII_REQUEST_SSEND;
   req.comm = comm;
   req.buf = buf;
   req.count = count;
   req.datatype = datatype;
   req.tag = tag;

   post_send (((MPII_Member **) comm->group->members) [dest], &req, msg,
         member);
   member = MPII_Me (comm);
   lock (member->mutex);
      while (!MPII_queue_search (&retry, &(member->queue), (void *)MPII_match_send,
                                 &req, &msg))
         wait (member->cond, member->mutex);
   unlock (member->mutex);

   return MPI_SUCCESS;
}


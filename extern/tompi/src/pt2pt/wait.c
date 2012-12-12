#include "mpii.h"

PUBLIC int MPI_Wait (MPI_Request *request, MPI_Status *status) 
{
   MPII_Member *me = MPII_Me (request->comm);
   MPII_Msg msg;
   int retry = 0, rval = MPI_SUCCESS;

   if (! request->active)
      return MPII_Error (request->comm, MPII_INACTIVE_REQ);

#if 0
   printf ("me is %p. queue is %p, qq is %p\n", me, &(me->queue), me->queue.q);
   if (me->queue.q == 0)
     exit (1);
#endif

   switch (request->type)
   {
      case MPII_REQUEST_SSEND:
         lock (me->mutex);
            while (!MPII_queue_search (&retry, &(me->queue), (void *)MPII_match_send,
                                       request, &msg))
               wait (me->cond, me->mutex);
         unlock (me->mutex);

         if (status != NULL)
         {
            status->MPI_SOURCE = MPI_ANY_SOURCE;
            status->MPI_TAG = MPI_ANY_TAG;
         }
         break;

      case MPII_REQUEST_RECV:
         lock (me->mutex);
            while (!MPII_queue_search (&retry, &(me->queue), (void *)MPII_match_recv,
                                       request, &msg))
               wait (me->cond, me->mutex);
         unlock (me->mutex);

         if (status != NULL)
         {
            status->MPI_SOURCE = msg.req->comm->group->rank;
            status->MPI_TAG = msg.req->tag;
            status->MPII_COUNT = msg.req->count *
                                 MPII_types[msg.req->datatype].size;
         }

         if (request->datatype != msg.req->datatype)
            rval = MPII_Error (request->comm, MPII_TYPE_MISMATCH);
         else if (request->count >= msg.req->count)
            memcpy (request->buf, msg.req->buf,
                  msg.req->count * MPII_types[request->datatype].size);
         else
         {
            memcpy (request->buf, msg.req->buf, request->count *
                  MPII_types[request->datatype].size);
            rval = MPII_Error (request->comm, MPII_OVERFLOW);
         }

         notify_sender (((MPII_Member **) msg.req->comm->group->members)
               [msg.req->comm->group->rank], msg, me);
         break;

      default: /* MPII_REQUEST_NULL */
         return MPII_Error (request->comm, MPII_NULL_REQ);
   }

   request->active = 0;
   if (! (request->persistent))
      *request = MPII_Request_null_val;
   return rval;
}


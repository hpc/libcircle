#include "mpii.h"

/* Match routines for use in MPII_queue_search */

/* Request is guaranteed to be a send */
PRIVATE int MPII_match_send (MPI_Request *request, MPII_Msg *msg)
{
   return (msg->type == TOOK_MSG && request == msg->req);
}

/* Request is guaranteed to be a receive */
PRIVATE int MPII_match_recv (MPI_Request *request, MPII_Msg *msg)
{
   return (msg->type == MSG_AVAIL
      && request->comm->context == msg->req->comm->context
      && ((request->tag == MPI_ANY_TAG && msg->req->tag > MPII_INTERNAL_TAGS)
        || request->tag == msg->req->tag)
      && (request->srcdest == MPI_ANY_SOURCE ||
            ((MPII_Member **) request->comm->group->members)
                [request->srcdest] ==
            ((MPII_Member **) msg->req->comm->group->members)
                [msg->req->comm->group->rank]));
}


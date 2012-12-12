#include "mpii.h"

PUBLIC int MPI_Start (MPI_Request *request)
{
   if (request->active)
      return MPII_Error (request->comm, MPII_ACTIVE_REQ);

   request->active = 1;

   switch (request->type)
   {
      case MPII_REQUEST_SSEND:
      {
         MPII_Msg msg;
         MPII_Member *target;
         post_send (((MPII_Member **) request->comm->group->members)
               [request->srcdest], request, msg, target);
         return MPI_SUCCESS;
      }

      case MPII_REQUEST_RECV:
         post_recv ();
         return MPI_SUCCESS;

      default: /* MPII_REQUEST_NULL */
         return MPII_Error (request->comm, MPII_NULL_REQ);
   }
}


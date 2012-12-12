#include "mpii.h"

static char *strings[MPII_MAX_ERROR+1] = {
   /* MPI_SUCCESS */           "No error (success)",
   /* MPII_NOT_INITIALIZED */  "MPI_Init() not called",
   /* MPII_BAD_ERRORCODE */    "Invalid errorcode argument",
   /* MPII_OUT_OF_MEMORY */    "Out of memory (malloc failed)",
   /* MPII_BAD_DATATYPE */     "Datatype out of bounds (invalid)",
   /* MPII_ACTIVE_REQ */       "MPI_Start: Active request",
   /* MPII_REQUEST_NULL */     "MPI_Start: REQUEST_NULL passed",
   /* MPII_RANK_RANGE */       "Rank is out of [0, communicator size)",
   /* MPII_TYPE_MISMATCH */    "Datatype mismatch between send and receive",
   /* MPII_OVERFLOW */         "Receive truncated data to specified amount",
   /* MPII_INACTIVE_REQ */     "MPI_Wait: Inactive request",
   /* MPII_COMM_NULL */        "MPI_COMM_NULL passed",
   /* MPII_GROUP_NULL */       "MPI_GROUP_NULL passed",
   /* MPII_HAVE_INITIALIZED */ "MPI_Init: Already initialized",
   /* MPII_ROOT_RANGE */       "Root is out of [0, communicator size)",
   /* MPII_BAD_TYPE_FOR_OP */  "Invalid datatype for collective operation",
   /* MPII_BAD_OP */           "Collective operation out of bounds (invalid)",
   /* MPII_BAD_KEY */          "Attribute keyval out of bounds (invalid)"
};

PUBLIC int MPI_Error_string (int errorcode, char *string, int *resultlen)
{
   if (errorcode < 0 || errorcode > MPII_MAX_ERROR)
      return MPII_Error (NULL, MPII_BAD_ERRORCODE);

   *resultlen = strlen (strings[errorcode]);
   strncpy (string, strings[errorcode], *resultlen);

   return MPI_SUCCESS;
}


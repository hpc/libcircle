#include "mpii.h"

static int classes[MPII_MAX_ERROR+1] = {
   /* MPI_SUCCESS */            MPI_SUCCESS,
   /* MPII_NOT_INITIALIZED */   MPI_ERR_OTHER,
   /* MPII_BAD_ERRORCODE */     MPI_ERR_ARG,
   /* MPII_OUT_OF_MEMORY */     MPI_ERR_OTHER,
   /* MPII_BAD_DATATYPE */      MPI_ERR_TYPE,
   /* MPII_ACTIVE_REQ */        MPI_ERR_REQUEST,
   /* MPII_REQUEST_NULL */      MPI_ERR_REQUEST,
   /* MPII_RANK_RANGE */        MPI_ERR_RANK,
   /* MPII_TYPE_MISMATCH */     MPI_ERR_TYPE,
   /* MPII_OVERFLOW */          MPI_ERR_TRUNCATE,
   /* MPII_INACTIVE_REQ */      MPI_ERR_REQUEST,
   /* MPII_COMM_NULL */         MPI_ERR_COMM,
   /* MPII_GROUP_NULL */        MPI_ERR_GROUP,
   /* MPII_HAVE_INITIALIZED */  MPI_ERR_OTHER,
   /* MPII_ROOT_RANGE */        MPI_ERR_ROOT,
   /* MPII_BAD_TYPE_FOR_OP */   MPI_ERR_TYPE,
   /* MPII_BAD_OP */            MPI_ERR_OP,
   /* MPII_BAD_KEY */           MPI_ERR_ARG
};

PUBLIC int MPI_Error_class (int errorcode, int *errorclass)
{
   if (errorcode < 0 || errorcode > MPII_MAX_ERROR)
      return MPII_Error (NULL, MPII_BAD_ERRORCODE);

   *errorclass = classes[errorcode];

   return MPI_SUCCESS;
}


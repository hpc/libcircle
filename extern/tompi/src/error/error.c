#include "mpii.h"

PUBLIC int MPII_Error (MPI_Comm comm, int code)
{
   if (comm == NULL || comm->errhandler == NULL)
      MPII_Fatal_error (&comm, &code);
   else
      comm->errhandler (&comm, &code);

   return code;
}


#include "mpii.h"
#include <stdio.h>

/* Code for MPI_ERRORS_ARE_FATAL handler */
PUBLIC void MPII_Fatal_error (MPI_Comm *comm, int *errorcode)
{
    char string[128];
    int len;

    if (MPI_Error_string (*errorcode, string, &len))
        fprintf (stderr,
            "MPI_ERRORS_ARE_FATAL: Error %d (MPI_Error_string failed)\n",
            *errorcode);
    else
    {
        string[len] = '\0';
        fprintf (stderr, "MPI_ERRORS_ARE_FATAL: Error: %s\n", string);
    }

    exit (1);
}


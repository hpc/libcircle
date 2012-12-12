#include "mpii.h"

PUBLIC void MPII_Exit (int rval)
{
   exit_thread ((void *) &rval);
}


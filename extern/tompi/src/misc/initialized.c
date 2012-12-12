#include "mpii.h"

static Key MPII_init_key;
static int init_init = 0;

PUBLIC void MPII_Initialized_init ()
{
  if (!init_init)
  {
    new_tsd (MPII_init_key);
    init_init = 1;
  }
}

PUBLIC int MPI_Initialized (long int *flag)
{
  MPII_Initialized_init ();
  *flag = (long int) get_tsd(MPII_init_key);
  return MPI_SUCCESS;
}

PRIVATE void MPII_Set_initialized ()
{
  MPII_Initialized_init ();
  set_tsd (MPII_init_key, (void *) 1);
}

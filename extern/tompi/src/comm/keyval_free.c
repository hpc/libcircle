#include "mpii.h"

PUBLIC int MPI_Keyval_free (int *keyval)
{
  DynamicId *did = get_tsd (MPII_attrib_did_key);
  check_keyval (*keyval, did, NULL);

  DID (MPII_Attrib, *keyval).freed = 1;
  if (DID (MPII_Attrib, *keyval).nref <= 0)
    DID_Give_free (*keyval);
  *keyval = MPI_KEYVAL_INVALID;

  return MPI_SUCCESS;
}

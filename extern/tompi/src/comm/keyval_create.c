#include "mpii.h"

PUBLIC int MPI_Keyval_create (MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval, void *extra_state)
{
  DynamicId *did = get_tsd (MPII_attrib_did_key);
  if (did->nfree < 0)
    DID_Grow ();
  *keyval = DID_Take_free ();
  DID (MPII_Attrib, *keyval).copy = (copy_fn ? copy_fn : MPI_NULL_FN);
  DID (MPII_Attrib, *keyval).delete = (delete_fn ? delete_fn : MPI_NULL_FN);
  DID (MPII_Attrib, *keyval).extra_state = extra_state;
  DID (MPII_Attrib, *keyval).nref = 0;
  return MPI_SUCCESS;
}

#include "mpii.h"

PRIVATE Key MPII_attrib_did_key;

PRIVATE void MPII_Attrib_init ()
{
  new_tsd (MPII_attrib_did_key);
}

PRIVATE void MPII_Local_attrib_init ()
{
  DynamicId *did = mymalloc (1, DynamicId);
  set_tsd (MPII_attrib_did_key, did);
  /*
  DID_Init (MPII_Attrib, ATTRIB_BLOCK_SIZE, MPII_NUM_BUILTIN_ATTRIBS);

  DID (MPII_Attrib, MPII_TOPOLOGY_ATTRIB).copy = MPI_NULL_FN;
  DID (MPII_Attrib, MPII_TOPOLOGY_ATTRIB).delete = MPI_NULL_FN;
  DID (MPII_Attrib, MPII_TOPOLOGY_ATTRIB).extra_state = NULL;
  DID (MPII_Attrib, MPII_TOPOLOGY_ATTRIB).nref =
      DID (MPII_Attrib, MPII_TOPOLOGY_ATTRIB).freed = 0;
  */
}

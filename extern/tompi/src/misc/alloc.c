#include "mpii.h"
#include <stdio.h>

PRIVATE void *MPII_Malloc (int size)
{
  void *p = (void *) malloc (size);
  if (p == NULL)
    MPII_Error (NULL, MPII_OUT_OF_MEMORY);
  else
    return p;
  return NULL;
}

PRIVATE void *MPII_Realloc (void *old, int size)
{
  void *p;
  if (old == NULL)
    p = (void *) malloc (size);
  else
    p = (void *) realloc (old, size);
  if (p == NULL)
    MPII_Error (NULL, MPII_OUT_OF_MEMORY);
  else
    return p;
  return NULL;
}

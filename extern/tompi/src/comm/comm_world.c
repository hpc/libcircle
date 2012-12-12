#include "mpii.h"

/* Returns the thread's MPI_COMM_WORLD communicator.
 * Used in the MPI_COMM_WORLD macro in mpi.h
 */
PUBLIC MPI_Comm MPII_comm_world (void)
{
  MPI_Comm world = (MPI_Comm) get_tsd (MPII_commworld_key);
  if (world == NULL)
    MPII_Error (NULL, MPII_NOT_INITIALIZED);
  if (world->group == NULL)
  {
    int i;
    printf ("ARGHhhhhhhhhhhhhhhh world->group is null! world=%p\n", world);
    
    for (i = 0; i < MPII_worlds[0]->group->size; i++)
    {
      printf ("world %d (%p) has group %p\n", i, MPII_worlds[i],
              MPII_worlds[i]->group);
      if (MPII_worlds[i]->group == NULL)
        printf ("!!!!!!!!!!!!!!!!!!!!!!!!NULL\n");
    }
    
    exit (1);
  }
  return world;
}


#include "mpii.h"
#include <stdio.h>

void MPII_Get_global_init (void);

/* For synchronizing with children */
static Mutex mutex1, mutex2;
static Cond cond1, cond2;
static int ninit, go;

/* This allows us to check whether we are the master.  It says whether the
 * master has called MPI_Init yet.
 */
static int master_initialized = 0;

static int orig_argc = 0;
static char **orig_argv = NULL;

static void *call_main (void *world)
{
  char **argv;
  int i;

  MPII_Slave ();

  /* Copy argv */
  argv = mymalloc (orig_argc+1, char *);
  for (i = 0; i < orig_argc; i++)
    argv[i] = orig_argv[i];
  argv[orig_argc] = NULL;

  /* If this is the first thread, tell the Get_global stuff our new id (since
   * we are now "renamed").  That is, make all the pre-init modifications to
   * global variables present in the new first thread.
   */
  if (((MPI_Comm) world)->group->rank == 0){
    MPII_Get_global_init_first ();
  }
  
  /* Define MPI_COMM_WORLD */
  set_tsd (MPII_commworld_key, world);
  
  /* Call user program */
  main (orig_argc, argv);
  
  return NULL;
}

/****************************************************************************/

PRIVATE void MPII_Master (int *argc, char ***argv, int part_of_MPI)
{
  MPII_Member **members;
  MPI_Context context;
  int i, error, nwait;
    
  MPII_Read_options (argc, argv);

  /* Get ready for slaves */
  master_initialized = 1;
  orig_argc = *argc;
  orig_argv = *argv;
  
  /* Initialize threads package */
  init_threads ();
    
  /* Initialize g2tsd_lib, as the first thread. */
  MPII_Get_global_init_first ();

  /* Initialize TSD support, if required */
#ifdef MPII_TSD
  MPII_Tsd_init (MPII_nthread);
#endif

  /* Globally initialize other "subpackages" of TOMPI. */
  MPII_Context_init ();
  MPII_Types_init ();
  MPII_Ops_init ();
  MPII_Attrib_init ();
  
  /* Define members */
  members = mymalloc (MPII_nthread, MPII_Member *);
  for (i = 0; i < MPII_nthread; i++)
  {
    members[i] = mymalloc (1, MPII_Member);
    if ((error = new_mutex (members[i]->mutex)))
      thread_error ("MPI_Init: Failed to create child mutex", error);
    if ((error = new_cond (members[i]->cond)))
      thread_error ("MPI_Init: Failed to create child condition", error);
    if (MPII_queue_init (&(members[i]->queue)))
      MPII_Error (NULL, MPII_OUT_OF_MEMORY);
  }
  new_tsd (MPII_me_key);
  
  /* Make MPI_COMM_WORLD for each thread */
  MPII_worlds = mymalloc (MPII_nthread, MPI_Comm);
  context = MPII_New_context ();
  for (i = 0; i < MPII_nthread; i++)
  {
    MPI_Comm world = mymalloc (1, MPII_Comm);
    MPII_worlds[i] = world;
    world->errhandler = MPI_ERRORS_ARE_FATAL;
    world->context = context;
    world->group = mymalloc (1, MPII_Group);
    world->group->rank = i;
    world->group->size = MPII_nthread;
    world->group->members = (void **) members;
    world->group->refcnt = 1;
  }
  new_tsd (MPII_commworld_key);

  /* Prepare to synchronize with children */
  if ((error = new_mutex (mutex1)))
    thread_error ("MPI_Init: Failed to create first mutex", error);
  if ((error = new_cond (cond1)))
    thread_error ("MPI_Init: Failed to create first condition", error);
  ninit = 0;
  if ((error = new_mutex (mutex2)))
    thread_error ("MPI_Init: Failed to create second mutex", error);
  if ((error = new_cond (cond2)))
    thread_error ("MPI_Init: Failed to create second condition", error);
  go = 0;

  /* Spawn threads */
  MPII_threads = mymalloc (MPII_nthread, Thread);
  if (part_of_MPI)
  {
#ifdef MPII_TSD
    MPII_Tsd_thread (thread_id (), MPII_nthread);
#endif
    MPII_threads[0] = thread_id ();
    set_tsd (MPII_commworld_key, MPII_worlds[0]);
    i = 1;
    nwait = MPII_nthread - 1;
  }
  else
  {
    i = 0;
    nwait = MPII_nthread;
  }
  for (; i < MPII_nthread; i++)
  {
    /* Spawn the child */
    if ((error = spawn_thread (call_main, (void *) MPII_worlds[i],
                              MPII_threads[i])))
    {
      thread_error ("MPI_Init: Failed to spawn thread", error);
      fprintf (stderr, "MPI_Init: I could only make a %d-process run\n",
               i);
      fprintf (stderr, "MPI_Init: Bailing out\n");
      
      /* Tell children about error and exit */
      lock (mutex2);
      go = -1;
      notify_all (cond2);
      unlock (mutex2);
      exit_thread (1);
    }

#   ifdef MPII_TSD
      MPII_Tsd_thread (MPII_threads[i]);
#   endif
  }
  
  /* Wait for threads to initialize */
#if 0
  printf ("Master: Obtaining mutex1 to wait for initialization signals\n");
#endif
  lock (mutex1);
  while (ninit < nwait)
  {
#if 0
    printf ("Master: so far, %d out of %d have initialized.\n", ninit,
            MPII_nthread);
#endif
    if ((error = wait (cond1, mutex1)))
      thread_error ("MPI_Init[0]: Failed at waiting for children to start up (ignoring)", error);
  }
  unlock (mutex1);
  
  /* Tell them to continue */
  ninit = 0;        /* for clean-up signals below */
  lock (mutex2);
  go = 1;
  notify_all (cond2);
  unlock (mutex2);
  
  /* Wait for clean-up signals */
  lock (mutex1);
  while (ninit < nwait)
    if ((error = wait (cond1, mutex1)))
      thread_error ("MPI_Init[0]: Failed at waiting for children to initialize (ignoring)", error);
  unlock (mutex1);
  
  /* Clean up */
  delete_mutex (mutex1);
  delete_cond (cond1);
  delete_mutex (mutex2);
  delete_cond (cond2);
}

/****************************************************************************/

PRIVATE void MPII_Slave ()
{
  int error;

  /* Signal the master that we have started up */
#if 0
  printf ("Slave: Obtaining mutex1 for notification of starting up\n");
#endif
  lock (mutex1);
  ninit++;
  if ((error = notify (cond1)))
    thread_error ("MPI_Init: Couldn't notify master about starting up", error);
  unlock (mutex1);
#if 0
  printf ("Slave: Notified that we have started up\n");
#endif
    
  /* Wait for a reply */
  lock (mutex2);
  while (!go)
    if ((error = wait (cond2, mutex2)))
      thread_error ("MPI_Init: Failed at waiting for master to give error code", error);
  unlock (mutex2);
  
  if (go < 0)       /* Check for error at master */
    exit_thread (1);
    
  /* Tell the master it is okay to delete the mutices and conds now */
  lock (mutex1);
  ninit++;
  if ((error = notify (cond1)))
    thread_error ("MPI_Init: Failed at telling master about initialization", error);
  if ((error = unlock (mutex1)))
    thread_error ("MPI_Init: Master deleted mutex1 too soon!", error);
}

/****************************************************************************/

PUBLIC int MPI_Init (int *argc, char ***argv)
{
  long int initialized;
  MPI_Group worldg;

  /* Make sure we don't get called more than once */
  MPI_Initialized (&initialized);
  if (initialized)
    return MPII_Error (NULL, MPII_HAVE_INITIALIZED);
  MPII_Set_initialized ();

  if (!master_initialized)
    MPII_Master (argc, argv, 1);
  /* Otherwise, MPII_Slave code has already been executed (by call_main(),
   * before even calling the user's main() program).
   */
  
  /* Set local "me" value */
  worldg = MPI_COMM_WORLD->group;
  set_tsd (MPII_me_key, ((MPII_Member **) (worldg->members))[worldg->rank]);

  /* Locally initialize other "subpackages" of TOMPI. */
  MPII_Local_attrib_init ();
  return 0;
}

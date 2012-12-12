/* Prototypes for all functions. */

/* Definition of PUBLIC and PRIVATE */
#ifndef PUBLIC
#define PUBLIC
#endif
#ifndef PRIVATE
#define PRIVATE
#endif

PUBLIC int MPI_Finalize (void);
PUBLIC double MPI_Wtime (void);
PRIVATE void MPII_Master (int *argc, char ***argv, int part_of_MPI);
PRIVATE void MPII_Slave ();
PUBLIC int MPI_Init (int *argc, char ***argv);
PRIVATE int MPII_queue_init (MPII_Msg_queue *qu);
PRIVATE int MPII_enqueue (MPII_Msg_queue *qu, MPII_Msg *data);
PRIVATE int MPII_queue_search (int *retry, MPII_Msg_queue *qu, void *match (void *, MPII_Msg *), void *arg, MPII_Msg *result);
PRIVATE void MPII_Tsd_master_init (Thread id, int n);
PRIVATE void MPII_Tsd_slave_init (Thread id);
PRIVATE int MPII_New_tsd (Key *key);
PRIVATE void *MPII_Get_tsd (Key *key);
PRIVATE int MPII_Set_tsd (Key *key, void *val);
PUBLIC int MPI_Initialized (long int *flag);
PRIVATE void MPII_Set_initialized ();
PRIVATE void *MPII_Malloc (int size);
PRIVATE void *MPII_Realloc (void *old, int size);
extern PRIVATE int MPII_nthread;
extern PRIVATE int MPII_Stack_size_val;
PUBLIC int *MPII_Stack_size_ptr (void);
extern PRIVATE int MPII_Num_proc_val;
PUBLIC int *MPII_Num_proc_ptr (void);
extern PRIVATE Key MPII_commworld_key;
extern PRIVATE Key MPII_me_key;
extern PRIVATE MPI_Request MPII_Request_null_val;
PUBLIC MPI_Request *MPII_Request_null_ptr (void);
extern PRIVATE MPI_Comm *MPII_worlds;
extern PRIVATE Thread *MPII_threads;
PUBLIC int MPI_Pcontrol (const int level, ...);
PRIVATE MPII_Member *MPII_Me (MPI_Comm comm);
PRIVATE void MPII_Thread_info ();
PUBLIC void MPII_Exit (int rval);
PRIVATE void MPII_Read_options (int *argc, char ***argv);
PUBLIC int MPI_Init (int *argc, char ***argv);
PUBLIC int MPI_Ssend_init (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
PUBLIC int MPI_Recv_init (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
PRIVATE int MPII_match_send (MPI_Request *request, MPII_Msg *msg);
PRIVATE int MPII_match_recv (MPI_Request *request, MPII_Msg *msg);
PUBLIC int MPI_Issend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
PUBLIC int MPI_Start (MPI_Request *request);
PUBLIC int MPI_Get_count (MPI_Status *status, MPI_Datatype datatype, int *count);
PUBLIC int MPI_Irecv (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
PUBLIC int MPI_Recv (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
PUBLIC int MPI_Ssend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
PUBLIC int MPI_Wait (MPI_Request *request, MPI_Status *status) ;
PUBLIC int MPI_Waitall (int count, MPI_Request *requests, MPI_Status *statuses);
PUBLIC void MPII_Do_nothing (MPI_Comm *comm, int *errorcode);
PUBLIC int MPII_Error (MPI_Comm comm, int code);
PUBLIC int MPI_Errhandler_get (MPI_Comm comm, MPI_Errhandler *errhandler);
PUBLIC int MPI_Errhandler_create (MPI_Handler_function *function, MPI_Errhandler *errhandler);
PUBLIC int MPI_Error_string (int errorcode, char *string, int *resultlen);
PUBLIC int MPI_Error_class (int errorcode, int *errorclass);
PUBLIC void MPII_Fatal_error (MPI_Comm *comm, int *errorcode);
PUBLIC int MPI_Errhandler_set (MPI_Comm comm, MPI_Errhandler errhandler);
PRIVATE void MPII_Context_init ();
PRIVATE MPI_Context MPII_New_context ();
PUBLIC int MPI_Type_size (MPI_Datatype datatype, int *size);
extern PRIVATE MPII_Datatype *MPII_types;
extern PRIVATE int MPII_ntypes;
PRIVATE void MPII_Types_init ();
PRIVATE int MPII_Is_captain (MPI_Group group);
PUBLIC int MPI_Group_rank (MPI_Group group, int *rank);
PUBLIC int MPI_Group_size (MPI_Group group, int *size);
PUBLIC int MPI_Group_compare (MPI_Group group1, MPI_Group group2, int *result);
PUBLIC int MPI_Group_translate_ranks (MPI_Group group1, int n, int *ranks1, MPI_Group group2, int *ranks2);
PUBLIC int MPI_Comm_group (MPI_Comm comm, MPI_Group *group);
PUBLIC int MPI_Comm_size (MPI_Comm comm, int *size);
PUBLIC MPI_Comm MPII_comm_world (void);
PUBLIC int MPI_Comm_dup (MPI_Comm comm, MPI_Comm *newcomm);
PUBLIC int MPI_NULL_FN ();
PUBLIC int MPI_DUP_FN (MPI_Comm *oldcomm, int *keyval, void *extra_state, void *attribute_val_in, void **attribute_val_out, int *flag);
PUBLIC int MPI_Comm_rank (MPI_Comm comm, int *rank);
extern PRIVATE Key MPII_attrib_did_key;
PRIVATE void MPII_Attrib_init ();
PRIVATE void MPII_Local_attrib_init ();
PUBLIC int MPI_Comm_compare (MPI_Comm comm1, MPI_Comm comm2, int *result);
PUBLIC int MPI_Keyval_create (MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval, void *extra_state);
PUBLIC int MPI_Keyval_free (int *keyval);
PUBLIC int MPI_Allreduce (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
PUBLIC int MPI_Reduce (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
PUBLIC int MPI_Bcast (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
PUBLIC int MPI_Barrier (MPI_Comm comm);
extern PRIVATE MPII_Op *MPII_ops;
extern PRIVATE int MPII_nops;
PRIVATE void MPII_Ops_init ();
PRIVATE void MPII_Sum (void *invec, void *inoutvec, int *len, MPI_Datatype *datatype);

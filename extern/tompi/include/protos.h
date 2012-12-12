/* Prototypes for all public functions. */

/* Definition of PUBLIC */
#ifndef PUBLIC
#define PUBLIC
#endif

PUBLIC int MPI_Finalize (void);
PUBLIC double MPI_Wtime (void);
PUBLIC int MPI_Init (int *argc, char ***argv);
PUBLIC int MPI_Initialized (long int *flag);
PUBLIC int *MPII_Stack_size_ptr (void);
PUBLIC int *MPII_Num_proc_ptr (void);
PUBLIC MPI_Request *MPII_Request_null_ptr (void);
PUBLIC int MPI_Pcontrol (const int level, ...);
PUBLIC void MPII_Exit (int rval);
PUBLIC int MPI_Init (int *argc, char ***argv);
PUBLIC int MPI_Ssend_init (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
PUBLIC int MPI_Recv_init (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
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
PUBLIC int MPI_Type_size (MPI_Datatype datatype, int *size);
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
PUBLIC int MPI_Comm_compare (MPI_Comm comm1, MPI_Comm comm2, int *result);
PUBLIC int MPI_Keyval_create (MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval, void *extra_state);
PUBLIC int MPI_Keyval_free (int *keyval);
PUBLIC int MPI_Allreduce (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
PUBLIC int MPI_Reduce (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
PUBLIC int MPI_Bcast (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
PUBLIC int MPI_Barrier (MPI_Comm comm);

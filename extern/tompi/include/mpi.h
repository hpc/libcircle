#ifndef __MPI_H__
#define __MPI_H__

/* Definitions specific to the thread system */
#include "threadspec.h"

/* Useful if you want to comment out code since it isn't yet supported by
 * TOMPI; just use #ifndef TOMPI ... #endif
 */
#define TOMPI

/* Mark that this implementation supports the MPII_Stack_size global variable
 * to specify the stack size of a process (in bytes, or 0 to use default).
 * Currently this means that the implementation is TOMPI.
 */
#define MPII_STACK_SIZE

/* Mark that this implementation supports the MPI_Num_proc global variable
 * to specify the number of processors to use.
 * Currently this means that the implementation is TOMPI.
 */
#define MPII_NUM_PROC

/* To avoid MPII_Stack_size and MPII_Num_proc from being modified from the
 * preprocessor, they are actually macros to call functions.
 */
#define MPII_Stack_size (*(MPII_Stack_size_ptr ()))
#define MPII_Num_proc   (*(MPII_Num_proc_ptr ()))

/* Error handlers */
struct MPII_Comm_STRUCT;
typedef void MPI_Handler_function (struct MPII_Comm_STRUCT **, int *, ...);
typedef MPI_Handler_function *MPI_Errhandler;
#define MPI_ERRORS_ARE_FATAL ((MPI_Errhandler) MPII_Fatal_error)
#define MPI_ERRORS_RETURN ((MPI_Errhandler) MPII_Do_nothing)
#define MPI_ERRHANDLER_NULL ((MPI_Errhandler) 0)

/* Structures */
typedef struct
{
    int rank, size, refcnt;
    void **members;      /* really a MPI_Member **  */
}
#ifdef MPI_INTERNAL
   MPII_Group,
#endif
*MPI_Group;

typedef unsigned int MPI_Context;
#define MPII_CONTEXT_TYPE MPI_UNSIGNED_INT

typedef struct MPII_Comm_STRUCT
{
   MPI_Group group;
   MPI_Context context;
   MPI_Errhandler errhandler;
} 
#ifdef MPI_INTERNAL
   MPII_Comm,
#endif
*MPI_Comm;

#define MPI_COMM_WORLD (MPII_comm_world ())

/* For group and communicator comparison */
#define MPI_IDENT 1
#define MPI_CONGRUENT 2
#define MPI_SIMILAR 3
#define MPI_UNEQUAL 4

#define MPI_UNDEFINED -10

/* Define exit to (indirectly) call thread_exit().  Also tell gthread.h about
 * this by defining MPII_THREAD_LOCAL_EXIT.
 */
#define MPII_THREAD_LOCAL_EXIT
#define exit(val) MPII_Exit(val)

/* This is perhaps temporary... */
#define MPI_Send MPI_Ssend
#define MPI_Send_init MPI_Ssend_init
#define MPI_Isend MPI_Issend

/* Data types */
typedef int MPI_Datatype;
#define MPI_CHAR 0
#define MPI_SHORT 1
#define MPI_SHORT_INT 1
#define MPI_INT 2
#define MPI_LONG 3
#define MPI_LONG_INT 3
#define MPI_LONG_LONG 4
#define MPI_LONG_LONG_INT 4
#define MPI_UNSIGNED 5
#define MPI_UNSIGNED_INT 5
#define MPI_UNSIGNED_CHAR 6
#define MPI_UNSIGNED_SHORT 7
#define MPI_UNSIGNED_SHORT_INT 7
#define MPI_UNSIGNED_LONG 8
#define MPI_UNSIGNED_LONG_INT 8
#define MPI_UNSIGNED_LONG_LONG 9
#define MPI_UNSIGNED_LONG_LONG_INT 9
#define MPI_FLOAT 10
#define MPI_DOUBLE 11
#define MPI_LONG_DOUBLE 12
#define MPI_BYTE 13
#define MPI_PACKED 14

/* Operations */
typedef void MPI_User_function (void *invec, void *inoutvec, int *len,
                                MPI_Datatype *datatype);
typedef int MPI_Op;
#define MPI_SUM 0

/* Attributes */
#define MPI_KEYVAL_INVALID -1
typedef int MPI_Copy_function (MPI_Comm *oldcomm, int *keyval,
    void *extra_state, void *attribute_val_in, void *attribute_val_out,
    int *flag);
typedef int MPI_Delete_function (MPI_Comm *comm, int *keyval,
                                 void *attribute_val, void *extra_state);

/* Request type */
typedef struct
{
   char active;
   char persistent;
   enum {MPII_REQUEST_NULL, MPII_REQUEST_SSEND, MPII_REQUEST_RECV} type;
   MPI_Comm comm;

   void *buf;
   int count;
   MPI_Datatype datatype;
   int srcdest;
   int tag;
} MPI_Request;
#define MPI_REQUEST_NULL (*(MPII_Request_null_ptr()))

/* Status structure */
typedef struct
{
   int MPI_SOURCE, MPI_TAG;
   int MPII_COUNT;
} MPI_Status;

/* Error classes */
#define MPI_SUCCESS 0
#define MPI_ERR_BUFFER 1
#define MPI_ERR_COUNT 2
#define MPI_ERR_TYPE 3
#define MPI_ERR_TAG 4
#define MPI_ERR_COMM 5
#define MPI_ERR_RANK 6
#define MPI_ERR_REQUEST 7
#define MPI_ERR_ROOT 8
#define MPI_ERR_GROUP 9
#define MPI_ERR_OP 10
#define MPI_ERR_TOPOLOGY 11
#define MPI_ERR_DIMS 12
#define MPI_ERR_ARG 13
#define MPI_ERR_UNKNOWN 14
#define MPI_ERR_TRUNCATE 15
#define MPI_ERR_OTHER 16
#define MPI_ERR_INTERN 17
#define MPI_ERR_LASTCODE MPI_ERR_INTERN

/* Wildcards */
#define MPI_ANY_TAG -1
#define MPI_ANY_SOURCE -1

/* Nulls */
#define MPI_PROC_NULL -2
#define MPI_COMM_NULL ((MPI_Comm) 0)
#define MPI_GROUP_NULL ((MPI_Group) 0)

/* Prototypes */
#ifndef MPI_INTERNAL
#   include "protos.h"
#endif

#endif

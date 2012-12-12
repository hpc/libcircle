/* Includes mpi.h, along with threads stuff and defines internal globals. */

#ifndef __MPII_H__
#define __MPII_H__

/******************** Some system-dependent information: ********************/

/* The "signed" keyword, if it isn't the default.  Putting signed in blows up
 * some compilers, so by default it is blank.
 */
#define MPII_SIGNED

/********************* No need to change anything below *********************/

#ifdef PROFILE
#   include "profile.h"    /* Add P prefix for profiling library */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MPI_INTERNAL
#include "mpi.h"
#include "gthread.h"   /* Must come after mpi.h for definitions of
                          MPII_Stack_size and such */

#undef MPII_Stack_size
#define MPII_Stack_size MPII_Stack_size_val
#undef MPII_Num_proc
#define MPII_Num_proc MPII_Num_proc_val
#undef MPI_REQUEST_NULL
#define MPI_REQUEST_NULL MPII_Request_null_val

/* Structures */

struct MPII_Member_STRUCT;
typedef struct
{
   enum {MSG_AVAIL, TOOK_MSG} type;
   MPI_Request *req;
} MPII_Msg;

typedef struct
{
   MPII_Msg *q;
   int *freelist;
   int *next;
   int max, nfree, head, tail, dirty, dirty_prev;
} MPII_Msg_queue;

typedef struct MPII_Member_STRUCT
{
   /* Thread thread; */
   Mutex mutex;
   Cond cond;
   MPII_Msg_queue queue;
} MPII_Member;

/* Dynamic ids */
typedef struct
{
  void *buf;
  int *freelist;
  int sizeof_type, num, nfree, block;
} DynamicId;
#define QUEUE_BLOCK_SIZE 64
#define ATTRIB_BLOCK_SIZE 8
#define ATTRIB_DID get_tsd (MPII_attrib_did_key)
#define DID_Next_block_size(size,block) ((size) + (block) - (size) % (block))
#define DID_Init(type,inblock,initnum) { \
  did->block = inblock; \
  did->num = initnum; \
  did->sizeof_type = sizeof (type); \
  did->buf = MPII_Malloc (initnum * did->sizeof_type); \
  did->freelist = NULL; \
  did->nfree = 0; \
}
#define DID_Grow() { \
  int i, j, size = did->num + did->nfree + did->block; \
  did->buf = MPII_Realloc (did->buf, size * did->sizeof_type); \
  did->freelist = myrealloc (did->freelist, size, int); \
  for (i = did->nfree, j = size - did->block; j < size; i++, j++) \
    did->freelist[i] = j; \
  did->nfree += did->block; \
}
#define DID_Grow_for(element) { \
  int i, j, size = DID_Next_block_size (element + 1, did->block); \
  if (did->num + did->nfree < size) { \
    did->buf = MPII_Realloc (did->buf, size * did->sizeof_type); \
    did->freelist = myrealloc (did->freelist, size, int); \
    for (i = did->nfree, j = did->num + did->nfree; j < size; i++, j++) \
      did->freelist[i] = j; \
    did->nfree += size - (did->num + did->nfree); \
  } \
}
#define DID_Take_free() (did->num++, did->freelist[--did->nfree])
#define DID_Give_free(element) (did->num--, did->freelist[did->nfree++] = element)
#define DID(type,element) (((type *) did->buf)[element])

/* Datatypes */
#define MPII_NUM_BUILTIN_TYPES 15
typedef struct
{
  int size;
} MPII_Datatype;

/* Operations */
#define MPII_NUM_BUILTIN_OPS 1
typedef struct
{
  MPI_User_function *f;
  int commute;
} MPII_Op;

/* Attributes */
#define MPII_NUM_BUILTIN_ATTRIBS 1
#define MPII_TOPOLOGY_ATTRIB 0
typedef struct
{
  MPI_Copy_function *copy;
  MPI_Delete_function *delete;
  void *extra_state;
  int nref, freed;
} MPII_Attrib;

/* Internal tags.  These are used specially, within the MPI implementation.
 * They do not conflict with user messages.  One restriction: you cannot
 * receive an internally tagged message with MPI_ANY_TAG, that is, MPI_ANY_TAG
 * is considered a user tag.
 */
#define MPII_INTERNAL_TAGS  -10
#define MPII_BCAST_TAG      -10
#define MPII_BARRIER_TAG1   -11
#define MPII_BARRIER_TAG2   -12
#define MPII_REDUCE_TAG     -13
#define MPII_ALLREDUCE_TAG1 -14
#define MPII_ALLREDUCE_TAG2 -15

#define mymalloc(num,type) \
   ((type *) MPII_Malloc ((num) * sizeof (type)))
#define myrealloc(var,num,type) \
   ((type *) MPII_Realloc (var, (num) * sizeof (type)))

/* Checks.  Careful if you have else's anywhere near these. */
#define check_coll_root(root,comm) \
  if (((root) < 0 || (root) > (comm)->group->size) && \
      (root) != MPI_PROC_NULL) \
    return MPII_Error (comm, MPII_ROOT_RANGE)
#define check_dest_rank(dest,comm) \
  if (((dest) < 0 || (dest) > (comm)->group->size) && \
      (dest) != MPI_PROC_NULL) \
    return MPII_Error (comm, MPII_RANK_RANGE)
#define check_source_rank(source,comm) \
  if (((source) < 0 || (source) > (comm)->group->size) && \
      (source) != MPI_ANY_SOURCE && (source) != MPI_PROC_NULL) \
    return MPII_Error (comm, MPII_RANK_RANGE)
#define check_comm(comm) \
  if ((comm) == MPI_COMM_NULL) \
    return MPII_Error (comm, MPII_COMM_NULL)
#define check_group(group,comm) \
  if ((group) == MPI_GROUP_NULL) \
    return MPII_Error (comm, MPII_GROUP_NULL)
#define check_datatype(datatype,comm) \
  if ((datatype) < 0 || (datatype) >= MPII_ntypes) \
    return MPII_Error (comm, MPII_BAD_DATATYPE)
#define check_op(op,comm) \
  if ((op) < 0 || (op) >= MPII_nops) \
    return MPII_Error (comm, MPII_BAD_OP)
#define check_keyval(key,did,comm) \
  if ((key) < 0 || (key) >= did->num || \
      (DID (MPII_Attrib, key).freed && DID (MPII_Attrib, key).nref <= 0)) \
    return MPII_Error (comm, MPII_BAD_KEYVAL)

/* Used for inter-thread communication */
#define post_send(target_in,request,msgvar,targetvar) \
      targetvar = target_in; \
      msgvar.type = MSG_AVAIL; \
      msgvar.req = request; \
      lock ((targetvar)->mutex); \
         MPII_enqueue (&((targetvar)->queue), &(msgvar)); \
         notify ((targetvar)->cond); \
      unlock ((targetvar)->mutex)
#define post_recv() 
#define notify_sender(src_in,msgvar,srcvar) \
      srcvar = src_in; \
      msgvar.type = TOOK_MSG; \
      /* msgvar.req = request; */ \
      lock ((srcvar)->mutex); \
         MPII_enqueue (&((srcvar)->queue), &(msgvar)); \
         notify ((srcvar)->cond); \
      unlock ((srcvar)->mutex)

/* Debug flags */
#define DEBUG_QUEUE_SEARCH 0
   /* 1 ==> show matches; 2 ==> show traversal through queue;
    * 3 ==> show dirty info
    */
#define DEBUG_ENQUEUE 0
   /* 1 ==> show enqueues; 2 ==> say where it went */

/* Errors */
/* MPI_SUCCESS (0) is an error in addition to an error class (see mpi.h) */
#define MPII_NOT_INITIALIZED      1
#define MPII_BAD_ERRORCODE        2
#define MPII_OUT_OF_MEMORY        3
#define MPII_BAD_DATATYPE         4
#define MPII_ACTIVE_REQ           5
#define MPII_NULL_REQ             6
#define MPII_RANK_RANGE           7
#define MPII_TYPE_MISMATCH        8
#define MPII_OVERFLOW             9
#define MPII_INACTIVE_REQ        10
#define MPII_COMM_NULL           11
#define MPII_GROUP_NULL          12
#define MPII_HAVE_INITIALIZED    13
#define MPII_ROOT_RANGE          14
#define MPII_BAD_TYPE_FOR_OP     15
#define MPII_BAD_OP              16
#define MPII_BAD_KEYVAL          17
/* don't forget: */
#define MPII_MAX_ERROR           17

/*some externs*/
extern void MPII_Get_global_init_first (void);
extern void MPII_Main(int argv, char **argc);

#include "iprotos.h"



#endif

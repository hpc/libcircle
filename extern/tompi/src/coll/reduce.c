#include "mpii.h"

/* This source file defines both MPI_Allreduce and MPI_Reduce.  When we want
 * to define MPI_Allreduce, ALLREDUCE is defined.
 */
#ifdef ALLREDUCE
# undef MPII_REDUCE_TAG
# define MPII_REDUCE_TAG MPII_ALLREDUCE_TAG1
#endif

#ifdef ALLREDUCE
PUBLIC int MPI_Allreduce (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
#define root 0
#else
PUBLIC int MPI_Reduce (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
#endif
{
  int rval = MPI_SUCCESS;

  check_comm (comm);
  check_coll_root (root, comm);
  check_datatype (datatype, comm);
  check_op (op, comm);

  if (comm->group->size == 1)
  {
    memcpy (recvbuf, sendbuf, count * MPII_types[datatype].size);
    return MPI_SUCCESS;
  }

  if (comm->group->rank == root)
  {
    int i;
    int len = count * MPII_types[datatype].size;
    void *tempbuf = MPII_Malloc (len);
#ifdef ALLREDUCE
    MPI_Request *reqs;
#endif

    if (MPII_ops[op].commute)
    {
      if ((rval = MPI_Recv (recvbuf, count, datatype, MPI_ANY_SOURCE,
                           MPII_REDUCE_TAG, comm, NULL)))
      {
        free (tempbuf);
        return rval;
      }
      (*(MPII_ops[op].f)) (sendbuf, recvbuf, &count, &datatype);

      for (i = 0; i < comm->group->size - 1; i++)
      {
        if ((rval = MPI_Recv (tempbuf, count, datatype, MPI_ANY_SOURCE,
                             MPII_REDUCE_TAG, comm, NULL)))
        {
          free (tempbuf);
          return rval;
        }
        (*(MPII_ops[op].f)) (tempbuf, recvbuf, &count, &datatype);
      }
    }
    else
    {
      if (root == 0)
        memmove (recvbuf, sendbuf, len);
      else if ((rval = MPI_Recv (recvbuf, count, datatype, 0,
                                MPII_REDUCE_TAG, comm, NULL)))
      {
        free (tempbuf);
        return rval;
      }

      for (i = 1; i < comm->group->size; i++)
      {
        if (root == i)
          memmove (tempbuf, sendbuf, len);
        else if ((rval = MPI_Recv (tempbuf, count, datatype, i,
                                  MPII_REDUCE_TAG, comm, NULL)))
        {
          free (tempbuf);
          return rval;
        }
        (*(MPII_ops[op].f)) (recvbuf, tempbuf, &count, &datatype);

        if (++i >= comm->group->size)
        {
          memmove (recvbuf, tempbuf, len);
          break;
        }

        if ((rval = MPI_Recv (recvbuf, count, datatype, i,
                             MPII_REDUCE_TAG, comm, NULL)))
        {
          free (tempbuf);
          return rval;
        }
        (*(MPII_ops[op].f)) (tempbuf, recvbuf, &count, &datatype);
      }
    }

    free (tempbuf);

#ifdef ALLREDUCE
    int j;
    reqs = mymalloc (comm->group->size - 1, MPI_Request);
    for (i = 0; i < root; i++)
      if ((rval = MPI_Isend (recvbuf, count, datatype, i, MPII_ALLREDUCE_TAG2,
                            comm, &(reqs[i]))))
      {
        free (reqs);
        return rval;
      }
    for (j = i++; i < comm->group->size; i++, j++)
      if ((rval = MPI_Isend (recvbuf, count, datatype, i, MPII_ALLREDUCE_TAG2,
                            comm, &(reqs[j]))))
      {
        free (reqs);
        return rval;
      }
    rval = MPI_Waitall (comm->group->size - 1, reqs, NULL);
    free (reqs);
#endif
  }
  else
  {
    if ((rval = MPI_Send (sendbuf, count, datatype, root, MPII_REDUCE_TAG, comm)))
      return rval;
#ifdef ALLREDUCE
    rval = MPI_Recv (recvbuf, count, datatype, root, MPII_ALLREDUCE_TAG2, comm,
            NULL);
#endif
  }
  return rval;
}


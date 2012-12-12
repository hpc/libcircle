/* Efficient queue package.  Allocates in chunks and uses free list.  Grows
 * dynamically and never shrinks.
 *
 * Routines return 1 if there was an allocation error or 0 on success.
 */

/* To take outside of this MPI thing:
 *    - remove the following include line
 *    - replace MPII_Msg_queue with general "Queue" type (see mpii.h)
 *    - replace MPII_Msg with desired Data type
 *    - remove MPII_ prefixes from routines (other than grow())
 */
#include "mpii.h"

#define type2str(type) ((type) == MSG_AVAIL ? "msg-avail" : "took-msg")

PRIVATE int MPII_queue_init (MPII_Msg_queue *qu)
{
   int i;

   qu->max = QUEUE_BLOCK_SIZE;
   qu->head = qu->tail = qu->dirty = qu->dirty_prev = -1;

   qu->q = (MPII_Msg *) malloc (sizeof (MPII_Msg) * QUEUE_BLOCK_SIZE);
   if (qu->q == NULL)
      return 1;
   qu->freelist = (int *) malloc (sizeof (int) * QUEUE_BLOCK_SIZE);
   if (qu->freelist == NULL)
      return 1;
   qu->next = (int *) malloc (sizeof (int) * QUEUE_BLOCK_SIZE);
   if (qu->next == NULL)
      return 1;

   qu->nfree = QUEUE_BLOCK_SIZE;
   for (i = 0; i < QUEUE_BLOCK_SIZE; i++)
      qu->freelist[i] = i;

   return 0;
}

static int grow (MPII_Msg_queue *qu)
{
    int i, newsize = qu->max + QUEUE_BLOCK_SIZE;
#if 0
    printf ("GROWING queue from %d to %d\n", qu->max, newsize);
#endif
    qu->q = (MPII_Msg *) realloc (qu->q, sizeof (MPII_Msg) * newsize);
    qu->freelist = (int *) realloc (qu->freelist, sizeof (int) * newsize);
    qu->next = (int *) realloc (qu->next, sizeof (int) * newsize);
    for (i = qu->max; i < newsize; i++)
        qu->freelist[qu->nfree++] = i;
    qu->max = newsize;
    /* Don't modify head or tail */

    return 0;
}

PRIVATE int MPII_enqueue (MPII_Msg_queue *qu, MPII_Msg *data)
{
   int pos;

   if (qu->nfree == 0)
      if (grow (qu))
         return 1;

   pos = qu->freelist[--qu->nfree];
   qu->q[pos] = *data;
   qu->next[pos] = -1;

#  if DEBUG_ENQUEUE >= 1
      printf ("enqueue: Adding %s.\n", type2str (qu->q[pos].type));
#  endif

   if (qu->head < 0)
   {
#     if DEBUG_ENQUEUE >= 2
         printf ("enqueue: That was the only element in the list (%d).\n", pos);
#     endif
      qu->head = qu->tail = pos;

      if (qu->dirty < 0)
      {
         qu->dirty = pos;
         qu->dirty_prev = -1;
      }
   }
   else
   {
#     if DEBUG_ENQUEUE >= 2
         printf ("enqueue: Inserted %d after %d (%s)\n", pos, qu->tail,
               type2str (qu->q[qu->tail].type));
#     endif
      qu->next[qu->tail] = pos;

      if (qu->dirty < 0)
      {
         qu->dirty = pos;
         qu->dirty_prev = qu->tail;
      }

      qu->tail = pos;
   }
   return 0;
}

/* Return value is different here: returns 1 if a match was found, 0 otherwise
 */
PRIVATE int MPII_queue_search (int *retry, MPII_Msg_queue *qu, void *match (void *, MPII_Msg *), void *arg, MPII_Msg *result)
{
   int pos, last;

   if (! (*retry))
   {
#     if DEBUG_QUEUE_SEARCH >= 3
         printf ("queue_search: Retry mode off, so starting at head %d\n",
               qu->head);
#     endif
      *retry = 1;
      qu->dirty = qu->head;
      qu->dirty_prev = -1;
   }
#  if DEBUG_QUEUE_SEARCH >= 3
      else
         printf ("queue_search: Retry mode on, so starting at dirty %d\n",
               qu->dirty);
#  endif

   for (last = qu->dirty_prev, pos = qu->dirty; pos >= 0; pos = qu->next[pos])
   {
#     if DEBUG_QUEUE_SEARCH >= 2
         printf ("queue_search: Searching through %d (%s, next: %d)\n",
               pos, type2str (qu->q[pos].type), qu->next[pos]);
#     endif

      if ((int *)match (arg, &(qu->q[pos])))
      {
#        if DEBUG_QUEUE_SEARCH >= 1
            printf ("queue_search: Matched %s.\n", type2str (qu->q[pos].type));
#        endif
         if (last < 0)
            qu->head = qu->next[pos];
         else
            qu->next[last] = qu->next[pos];
         if (pos == qu->tail)
            qu->tail = last;
         qu->freelist[qu->nfree++] = pos;
         qu->dirty = qu->next[pos];
         qu->dirty_prev = last;
         *result = qu->q[pos];
         return 1;
      }
      last = pos;
   }

   qu->dirty = qu->dirty_prev = -1;
   return 0;
}


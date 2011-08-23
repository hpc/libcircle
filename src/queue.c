/*
 * This file contains functions related to the local queue structure.
 */

#include <stdio.h>
#include <string.h>
#include "libcircle.h"
#include "queue.h"
#include "log.h"

/*
 * Dump the raw contents of the queue structure.
 */
void CIRCLE_queue_dump(CIRCLE_queue_t *qp)
{
   int i = 0;
   char * p = qp->base;

   while(p++ != (qp->strings[qp->count-1]+strlen(qp->strings[qp->count-1])))
       if(i++ % 120 == 0) LOG(LOG_DBG, "%c\n", *p); else LOG(LOG_DBG, "%c",*p);
}

/*
 * Pretty-print the queue data structure.
 */
void CIRCLE_queue_print(CIRCLE_queue_t *qp)
{
    int i = 0;

    for(i = 0; i < qp->count; i++)
       LOG(LOG_DBG, "\t[%p][%d] %s\n",qp->strings[i],i,qp->strings[i]);

    LOG(LOG_DBG, "\n");
}

/*
 * Push the specified string onto the work queue.
 */
int CIRCLE_queue_push(CIRCLE_queue_t *qp, char *str)
{
    //LOG)"Count: %d, Start: %p, End: %p MAX_STRING_LEN: %d, Diff: %lu\n",qp->count,qp->base,qp->end,MAX_STRING_LEN,qp->end-qp->base);
    assert(strlen(str) > 0);

    if(qp->count > 1)
        assert(qp->strings[qp->count-1] + MAX_STRING_LEN < qp->end);

    qp->strings[qp->count] = qp->head; 

    /* copy the string */
    strcpy(qp->head, str);
    assert(strlen(qp->head) < MAX_STRING_LEN);

    /* Make head point to the character after the string */
    qp->head = qp->head + strlen(qp->head) + 1;
    
    /* Make the head point to the next available memory */
    qp->count = qp->count + 1;

    LOG(LOG_DBG, "Push: %s\tLength: %lu\tBase: %p\tString: %p\tCount :%d\tDiff: %lu\n", \
        qp->head, strlen(qp->head), qp->base, qp->head, \
        qp->count, qp->strings[qp->count-1] - qp->strings[qp->count-2]);

    return 0;
}

/*
 * Removes an item from the queue and returns a copy.
 */
int CIRCLE_queue_pop(CIRCLE_queue_t *qp, char *str)
{
    if(qp->count == 0)
        return 0;

    /* Copy last element into str */
    strcpy(str,qp->strings[qp->count-1]);
    qp->count = qp->count - 1;

    return 0;
}

/* EOF */

/*
 * This file contains functions related to the local queue structure.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "libcircle.h"
#include "queue.h"
#include "log.h"

CIRCLE_queue_t * CIRCLE_queue_init(void)
{
    CIRCLE_queue_t * qp;

    qp = (CIRCLE_queue_t *) malloc(sizeof(CIRCLE_queue_t));
    qp->base = (char *) malloc(sizeof(char) * MAX_STRING_LEN * INITIAL_QUEUE_SIZE);
    qp->strings = (char **) malloc(sizeof(char*) * INITIAL_QUEUE_SIZE);

    if(!qp || !qp->base || !qp->strings) {
        LOG(LOG_ERR, "Failed to allocate a basic queue structure.");
    }

    return qp;
}

int CIRCLE_queue_free(CIRCLE_queue_t *qp)
{
    int i = 0;

    if(qp) {
        if(qp->strings) {
            LOG(LOG_DBG, "Freeing the queue strings array.");

            while(qp->strings[i])  {
                LOG(LOG_DBG, "Freeing a string element (%d).", i);
                free(qp->strings[i]);
                i++;
            }
        }

        if(qp->base) {
            LOG(LOG_DBG, "Freeing the queue base pointer.");
            free(qp->base);
        }

        LOG(LOG_DBG, "Freeing a queue pointer.");
        free(qp);
    } else {
        LOG(LOG_ERR, "Attempted to free a null queue structure.");
        return -1;
    }

    return 1;
}

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
    if(strlen(str) <= 0) {
        LOG(LOG_ERR, "Attempted to push an empty string onto a queue.");
        return -1;
    }

    if(qp->count > 1) {
        if(qp->strings[qp->count-1] + MAX_STRING_LEN >= qp->end) {
            LOG(LOG_ERR, "Size of queue not large enough to push another value.");
            return -1;
        }
    }

    /* Set our write location to the end of the current strings array. */
    qp->strings[qp->count] = qp->head; 

    if(strlen(str) > MAX_STRING_LEN) {
        LOG(LOG_ERR, "Attempted to push a value that was larger than expected.");
        return -1;
    }

    /* Copy the string. */
    strcpy(qp->head, str);

    /*
     * Make head point to the character after the string (strlen doesn't
     * include a trailing null).
     */
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
    if(!qp) {
        LOG(LOG_ERR, "Attempted to pop from an invalid queue.");
        return 0;
    }

    if(qp->count < 1) {
        LOG(LOG_DBG, "Attempted to pop from an empty queue.");
        return 0;
    }

    if(!str) {
        LOG(LOG_ERR, "You must allocate a buffer for storing the result.");
        return 0;
    }

    /* Copy last element into str */
    strcpy(str, qp->strings[qp->count-1]);
    qp->count = qp->count - 1;

    return 0;
}

/* EOF */

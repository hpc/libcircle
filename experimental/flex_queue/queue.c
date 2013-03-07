/*
 * Implementation of a splittable queue structure.
 */

#include <stdlib.h>

#include "queue.h"
#include "log.h"

int CIRCLE_internal_queue_from_buffer(intptr_t *buf, \
                                      CIRCLE_internal_queue_t *queue)
{
    /* TODO */
}

int CIRCLE_internal_queue_to_buffer(CIRCLE_internal_queue_t *queue, \
                                    intptr_t *buf)
{
    /* TODO */
}

int CIRCLE_internal_queue_alloc(CIRCLE_internal_queue_t *queue)
{
     if(queue != NULL) {
         LOG(CIRCLE_LOG_FATAL, "Attempted to use allocated memory.");
         return -1;
     }

     queue = (CIRCLE_internal_queue_t *) malloc(sizeof(CIRCLE_internal_queue_t));
     if(queue == NULL) {
         LOG(CIRCLE_LOG_FATAL, "Allocating an internal queue failed.");
         return -1;
     }

     queue->item_count = 0;
     queue->ptr_pool_count = 0;

     queue->items = NULL;

     return 1;
}

int CIRCLE_internal_queue_free(CIRCLE_internal_queue_t *queue)
{
    if(queue == NULL) {
        LOG(CIRCLE_LOG_FATAL, "Attempted to free a null internal queue.");
        return -1;
    }

    CIRCLE_internal_queue_item_t* curr_item = queue->items;
    while(curr_item != NULL) {
        free(curr_item);
        curr_item++;
    }

    return 1;
}

int CIRCLE_internal_queue_ptr_expand(CIRCLE_internal_queue_t *queue)
{
    if(queue == NULL) {
        LOG(CIRCLE_LOG_FATAL, "Attempted to expand an unallocated queue.");
        return -1;
    }

    if(queue->ptr_pool_count < 1) {
        queue->items = (intptr_t *) malloc( \
            sizeof(intptr_t) * CIRCLE_INTERNAL_QUEUE_REALLOC);
        queue->ptr_pool_count = CIRCLE_INTERNAL_QUEUE_REALLOC;

        if(queue->items == NULL) {
            LOG(CIRCLE_LOG_FATAL, "Failed to allocate pointers for queue items.");
            return -1;
        }
    } else {
        queue->items = (intptr_t *) realloc(queue->items, queue->ptr_pool_count * 2);

        if(queue->items == NULL) {
            LOG(CIRCLE_LOG_FATAL, "Failed to reallocate allocate pointers for queue items.");
            return -1;
        }
    }
}

int CIRCLE_internal_queue_push(CIRCLE_internal_queue_t *queue, \
                               intptr_t item, size_t item_length)
{
    if((queue->item_count + 1) < queue->ptr_pool_count) {
        if(!CIRCLE_internal_queue_ptr_expand(queue)) {
            LOG(CIRCLE_LOG_FATAL, "Failed to expand the queue pointer pool.");
            return -1;
        }
    }

    queue->items[queue->item_count] = (CIRCLE_internal_queue_item_t *) \
        malloc(sizeof(CIRCLE_internal_queue_item_t));

    if(queue->items[queue->item_count] == NULL) {
        LOG(CIRCLE_LOG_FATAL, "Failed to allocate an internal queue item.");
        return -1;
    }

    (queue->items[queue->item_count])->length = item_length;
    (queue->items[queue->item_count])->data = item;

    queue->item_count++;
}

int CIRCLE_internal_queue_pop(CIRCLE_internal_queue_t *queue, \
                              CIRCLE_internal_queue_item_t *item)
{
    if(queue == NULL) {
        LOG(CIRCLE_LOG_FATAL, "Attempted to pop an item from a null queue.");
        return -1;
    }

    if(queue->item_count < 1) {
        LOG(CIRCLE_LOG_DBG, "Attempted to pop and item from an empty queue.");
        return 0;
    }

    queue->item_count--;
    item = queue->items[queue->item_count];

    if(item == NULL) {
        LOG(CIRCLE_LOG_DBG, "Poping a null item.");
    }

    return 1;
}

int CIRCLE_internal_queue_split(CIRCLE_internal_queue_t *a, \
                                CIRCLE_internal_queue_t *b, int options)
{
    if(a == NULL && b == NULL) {
        LOG(CIRCLE_LOG_FATAL, "Attempted to split two null queues.");
        return -1;
    }

    if(a != NULL && b != NULL) {
        LOG(CIRCLE_LOG_FATAL, "Attempted to split two allocated queues.");
        return -1;
    }

    from_queue = a == NULL ? b : a;
    to_queue = a == NULL ? a : b;

    /* TODO */
}

/* EOF */

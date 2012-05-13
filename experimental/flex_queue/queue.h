#ifndef _CIRCLE_INTERNAL_QUEUE_H
#define _CIRCLE_INTERNAL_QUEUE_H

size_t CIRCLE_INTERNAL_QUEUE_REALLOC = 50;

/*
 * A item in the internal queue data structure.
 */
typedef struct {
    size_t length;
    intptr_t* data;
} CIRCLE_internal_queue_item_t;

/*
 * An internal queue data structure.
 */
typedef struct {
    size_t item_count;
    size_t ptr_pool_count;
    CIRCLE_internal_queue_item_t** items;
} CIRCLE_internal_queue_t;

#endif /* _CIRCLE_INTERNAL_QUEUE_H */

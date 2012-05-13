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

int CIRCLE_internal_queue_alloc(CIRCLE_internal_queue_t *queue);
int CIRCLE_internal_queue_free(CIRCLE_internal_queue_t *queue);

int CIRCLE_internal_queue_ptr_expand(CIRCLE_internal_queue_t *queue);

int CIRCLE_internal_queue_from_buffer(intptr_t *buf, \
                                      CIRCLE_internal_queue_t *queue);
int CIRCLE_internal_queue_to_buffer(CIRCLE_internal_queue_t *queue, \
                                    intptr_t *buf);

int CIRCLE_internal_queue_push(CIRCLE_internal_queue_t *queue, \
                               intptr_t item, size_t item_length);
int CIRCLE_internal_queue_pop(CIRCLE_internal_queue_t *queue, \
                              CIRCLE_internal_queue_item_t *item);

int CIRCLE_internal_queue_split(CIRCLE_internal_queue_t *a, \
                                CIRCLE_internal_queue_t *b, int options);

#endif /* _CIRCLE_INTERNAL_QUEUE_H */

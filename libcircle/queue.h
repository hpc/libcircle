#ifndef INTERNAL_QUEUE_H
#define INTERNAL_QUEUE_H

#include<stdint.h>

/* The initial queue size for malloc. */
#ifndef CIRCLE_INITIAL_INTERNAL_QUEUE_SIZE
#define CIRCLE_INITIAL_INTERNAL_QUEUE_SIZE 4096
#endif

typedef struct CIRCLE_internal_queue_t {
    char* base;         /* Base of the memory pool */
    size_t bytes;       /* current capacity of queue in bytes */
    uintptr_t head;     /* The location of the next free byte */
    uintptr_t* strings; /* The string data */
    int32_t str_count;  /* The maximum number of strings the queue can hold */
    int32_t count;      /* The number of actively queued strings */
} CIRCLE_internal_queue_t;

CIRCLE_internal_queue_t* CIRCLE_internal_queue_init(void);
int8_t CIRCLE_internal_queue_free(CIRCLE_internal_queue_t* qp);

int8_t CIRCLE_internal_queue_push(CIRCLE_internal_queue_t* qp, char* str);
int8_t CIRCLE_internal_queue_pop(CIRCLE_internal_queue_t* qp, char* str);

void CIRCLE_internal_queue_dump(CIRCLE_internal_queue_t* qp);
void CIRCLE_internal_queue_print(CIRCLE_internal_queue_t* qp);

int8_t CIRCLE_internal_queue_write(CIRCLE_internal_queue_t* qp, int rank);
int8_t CIRCLE_internal_queue_read(CIRCLE_internal_queue_t* qp, int rank);
int8_t CIRCLE_internal_queue_extend(CIRCLE_internal_queue_t* qp, size_t size);
int8_t CIRCLE_internal_queue_str_extend(CIRCLE_internal_queue_t* qp, int32_t count);

#endif /* QUEUE_H */

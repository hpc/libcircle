#ifndef INTERNAL_QUEUE_H
#define INTERNAL_QUEUE_H

#include<stdint.h>

/* The initial queue size for malloc. */
#ifndef CIRCLE_INITIAL_INTERNAL_QUEUE_SIZE
#define CIRCLE_INITIAL_INTERNAL_QUEUE_SIZE 4096
#endif

typedef struct CIRCLE_internal_queue_t {
    char* base;     /* Base of the memory pool */
    char* end;      /* End of the memory pool */
    uintptr_t next;     /* The location of the next string */
    uintptr_t head;     /* The location of the next free byte */
    uintptr_t* strings; /* The string data */
    uint32_t str_count;
    uint32_t count;      /* The number of strings */
} CIRCLE_internal_queue_t;

CIRCLE_internal_queue_t* CIRCLE_internal_queue_init(void);
int8_t CIRCLE_internal_queue_free(CIRCLE_internal_queue_t* qp);

int8_t CIRCLE_internal_queue_push(CIRCLE_internal_queue_t* qp, char* str);
int8_t CIRCLE_internal_queue_pop(CIRCLE_internal_queue_t* qp, char* str);

void CIRCLE_internal_queue_dump(CIRCLE_internal_queue_t* qp);
void CIRCLE_internal_queue_print(CIRCLE_internal_queue_t* qp);

int8_t CIRCLE_internal_queue_write(CIRCLE_internal_queue_t* qp, int rank);
int8_t CIRCLE_internal_queue_read(CIRCLE_internal_queue_t* qp, int rank);
int8_t CIRCLE_internal_queue_extend(CIRCLE_internal_queue_t* qp);
int8_t CIRCLE_internal_queue_str_extend(CIRCLE_internal_queue_t* qp, int new_size);

#endif /* QUEUE_H */

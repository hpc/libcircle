#ifndef QUEUE_H
#define QUEUE_H

/* The initial queue size for malloc. */
#ifndef CIRCLE_INITIAL_QUEUE_SIZE
#define CIRCLE_INITIAL_QUEUE_SIZE 400000
#endif

typedef struct CIRCLE_queue_t {
    char* base;     /* Base of the memory pool */
    char* end;      /* End of the memory pool */
    char* next;     /* The location of the next string */
    char* head;     /* The location of the next free byte */
    char** strings; /* The string data */
    int count;      /* The number of strings */
} CIRCLE_queue_t;

CIRCLE_queue_t* CIRCLE_queue_init(void);
int  CIRCLE_queue_free(CIRCLE_queue_t* qp);

int CIRCLE_queue_push(CIRCLE_queue_t* qp, char* str);
int CIRCLE_queue_pop(CIRCLE_queue_t* qp, char* str);

void CIRCLE_queue_dump(CIRCLE_queue_t* qp);
void CIRCLE_queue_print(CIRCLE_queue_t* qp);

int  CIRCLE_queue_write(CIRCLE_queue_t* qp, int rank);
int  CIRCLE_queue_read(CIRCLE_queue_t* qp, int rank);

#endif /* QUEUE_H */

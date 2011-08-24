#ifndef QUEUE_H
#define QUEUE_H

typedef struct CIRCLE_queue_t
{
    char *base;     /* Base of the memory pool */
    char *end;      /* End of the memory pool */
    char *next;     /* The location of the next string */
    char *head;     /* The location of the next free byte */
    char **strings; /* The string data */
    int count;      /* The number of strings */
    int num_stats;
} CIRCLE_queue_t;

/*
 * Initialize a queue.
 */
CIRCLE_queue_t * CIRCLE_queue_init(void);

/*
 * Free a queue.
 */
int CIRCLE_queue_free(CIRCLE_queue_t *qp);

/*
 * Dump the raw contents of the local queue structure.
 */
void CIRCLE_queue_dump( CIRCLE_queue_t *qp);

/*
 * Pretty-print the contents of the local queue structure.
 */
void CIRCLE_queue_print( CIRCLE_queue_t *qp );

/*
 * Pushes the specified string onto the work queue.
 */
int CIRCLE_queue_push( CIRCLE_queue_t *qp, char *str );

/*
 * Removes a string from the queue and returns a copy of it.
 */
int CIRCLE_queue_pop( CIRCLE_queue_t * qp, char *str );

#endif /* QUEUE_H */

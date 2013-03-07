#ifndef LIB_H
#define LIB_H

#include <config.h>

#include "libcircle.h"
#include "queue.h"

typedef struct CIRCLE_input_st {
    CIRCLE_cb create_cb;
    CIRCLE_cb process_cb;
    MPI_Comm* work_comm;
    MPI_Comm* token_comm;
    int options;
    CIRCLE_internal_queue_t* queue;
} CIRCLE_input_st;

void CIRCLE_checkpoint(void);
void CIRCLE_read_restarts(void);
void CIRCLE_abort(void);

#endif /* LIB_H */

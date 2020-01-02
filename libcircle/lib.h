#ifndef LIB_H
#define LIB_H

#include <config.h>

#include "libcircle.h"
#include "queue.h"

typedef struct CIRCLE_input_st {
    CIRCLE_cb create_cb;
    CIRCLE_cb process_cb;

    CIRCLE_cb_reduce_init_fn reduce_init_cb;
    CIRCLE_cb_reduce_op_fn   reduce_op_cb;
    CIRCLE_cb_reduce_fini_fn reduce_fini_cb;
    void*  reduce_buf;
    size_t reduce_buf_size;
    int reduce_period;

    MPI_Comm comm;

    int options;

    /* width of internal communication k-ary tree */
    int tree_width;

    CIRCLE_internal_queue_t* queue;
} CIRCLE_input_st;

#endif /* LIB_H */

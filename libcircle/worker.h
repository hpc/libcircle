#ifndef WORKER_H
#define WORKER_H

#include "libcircle.h"

#define LIBCIRCLE_MPI_ERROR 32

int8_t CIRCLE_worker();
int8_t _CIRCLE_read_restarts();
int8_t _CIRCLE_checkpoint();

#endif /* WORKER_H */

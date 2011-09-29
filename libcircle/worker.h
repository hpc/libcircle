#ifndef WORKER_H
#define WORKER_H

#include "libcircle.h"

#define LIBCIRCLE_MPI_ERROR 32

int CIRCLE_worker();
int _CIRCLE_read_restarts();
int _CIRCLE_checkpoint();

#endif /* WORKER_H */

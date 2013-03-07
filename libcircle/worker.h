#ifndef WORKER_H
#define WORKER_H

#include "libcircle.h"
#include "token.h"

#define LIBCIRCLE_MPI_ERROR 32

int8_t CIRCLE_worker();
int8_t _CIRCLE_read_restarts();
int8_t _CIRCLE_checkpoint();
void CIRCLE_reset_request_vector(CIRCLE_state_st* st);

#endif /* WORKER_H */

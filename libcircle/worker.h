#ifndef WORKER_H
#define WORKER_H

#include "libcircle.h"
#include "token.h"

#define LIBCIRCLE_MPI_ERROR 32

int8_t CIRCLE_worker(void);
int8_t _CIRCLE_read_restarts(void);
int8_t _CIRCLE_checkpoint(void);
void CIRCLE_reset_request_vector(CIRCLE_state_st* st);

/* TODO: move me to a util file */
void CIRCLE_free(void* ptr);

#endif /* WORKER_H */

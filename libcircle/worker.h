#ifndef WORKER_H
#define WORKER_H

#include "libcircle.h"
#include "token.h"

#ifndef MPI_MAX_OBJECT_NAME
  #define MPI_MAX_OBJECT_NAME (64)
#endif

#ifndef MPI_MAX_ERROR_STRING
  #define MPI_MAX_ERROR_STRING (256)
#endif

#define LIBCIRCLE_MPI_ERROR 32

int8_t CIRCLE_worker();
int8_t _CIRCLE_read_restarts();
int8_t _CIRCLE_checkpoint();
void CIRCLE_reset_request_vector(CIRCLE_state_st* st);

#endif /* WORKER_H */

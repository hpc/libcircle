#ifndef LOG_H
#define LOG_H

#include "libcircle.h"

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define LOG(level, ...) do {  \
        if (level <= CIRCLE_debug_level) { \
            fprintf(CIRCLE_debug_stream, "%d:%d:%s:%d:", (int)time(NULL), \
                    CIRCLE_global_rank, __FILE__, __LINE__); \
            fprintf(CIRCLE_debug_stream, __VA_ARGS__); \
            fprintf(CIRCLE_debug_stream, "\n"); \
            fflush(CIRCLE_debug_stream); \
        } \
    } while (0)

extern FILE* CIRCLE_debug_stream;
extern enum CIRCLE_loglevel CIRCLE_debug_level;
extern int32_t CIRCLE_global_rank;

#endif /* LOG_H */

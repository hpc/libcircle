#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#ifndef LIBCIRCLE_LOGLEVEL
    #define LIBCIRCLE_LOGLEVEL 5
#endif

#define LOG_FATAL (1)
#define LOG_ERR   (2)
#define LOG_WARN  (3)
#define LOG_INFO  (4)
#define LOG_DBG   (5)

#define LOG(level, ...) do {  \
        if (level <= CIRCLE_debug_level) { \
            fprintf(CIRCLE_debug_stream, "%d %d:%s:%d:", time(NULL),CIRCLE_global_rank, \
                __FILE__, __LINE__); \
            fprintf(CIRCLE_debug_stream, __VA_ARGS__); \
            fprintf(CIRCLE_debug_stream, "\n"); \
            fflush(CIRCLE_debug_stream); \
        } \
    } while (0)

extern FILE *CIRCLE_debug_stream;
extern int CIRCLE_debug_level;
extern int CIRCLE_global_rank;

#endif /* LOG_H */

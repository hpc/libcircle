#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define LOG_FATAL (1)
#define LOG_ERR   (2)
#define LOG_WARN  (3)
#define LOG_INFO  (4)
#define LOG_DBG   (5)

#define LOG(level, ...) do {  \
        if (level <= debug_level) { \
            fprintf(dbgstream,"%s:%d:", __FILE__, __LINE__); \
            fprintf(dbgstream, __VA_ARGS__); \
            fprintf(dbgstream, "\n"); \
            fflush(dbgstream); \
        } \
    } while (0)

extern FILE *dbgstream;
extern int  debug_level;

#endif /* LOG_H */

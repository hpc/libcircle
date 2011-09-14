#ifndef DSTAT_H
#define DSTAT_H

#include <libcircle.h>

void add_objects(CIRCLE_handle *handle);
void process_objects(CIRCLE_handle *handle);
int dstat_create_redis_attr_cmd(char *buf, struct stat *st, char *filename, char *filekey);
int dstat_redis_run_zadd(char *filekey, long val, char *zset, char *filename);
void dstat_redis_run_cmd(char *cmd, char *filename);
int dstat_redis_keygen(char *buf, char *filename);
void print_usage(char **argv);

#endif /* DSTAT_H */

#ifndef DSTAT_PLUGINS_H
#define DSTAT_PLUGINS_H

#include <sys/stat.h>

typedef struct DSTAT_plugin_ctx_st
{
   DSTAT_plugin_st **registered_plugins;
   int plugin_count;
} DSTAT_plugin_ctx_st;

typedef struct DSTAT_plugin_st
{
    void (*handle_output)(stat *st);
    char *trigger;
    int id;
} DSTAT_plugin_st;

#endif /* DSTAT_PLUGINS_H */

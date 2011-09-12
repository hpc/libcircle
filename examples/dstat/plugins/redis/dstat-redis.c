/*
 * Register a plugin with dstat that sends stat output to redis.
 */

#include <dstat-plugins.h>

void
dstat_handle_output_redis(stat *st)
{
    /* FIXME: not implemented */
}

void
dstat_plugin_register(DSTAT_plugin_ctx_st *plugin_ctx)
{
    DSTAT_plugin_st redis_plugin;
    int plugin_id = plugin_ctx->plugin_count;

    redis_plugin.trigger = "output=redis";
    redis_plugin.description = "A plugin to send dstat output to redis.";

    redis_plugin.handle_output = &dstat_handle_output_redis;
    redis_plugn.id = plugin_id;

    plugin_ctx->registered_plugins[plugin_id] = &redis_plugin;
    plugin_ctx->plugin_count++;
}

/* EOF */

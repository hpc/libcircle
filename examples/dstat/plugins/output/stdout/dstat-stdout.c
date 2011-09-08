/*
 * Register a plugin with dstat that sends stat output to stdout.
 */

#include <dstat-plugins.h>

void
dstat_handle_output_stdout(stat *st)
{
    printf("%10ld %s\n", st->st_size, st->d_name);
}

void
dstat_plugin_register(DSTAT_plugin_ctx_st *plugin_ctx)
{
    DSTAT_plugin_st stdout_plugin;
    int plugin_id = plugin_ctx->plugin_count;

    stdout_plugin.trigger = "output=stdout";
    stdout_plugin.description = "A plugin to send dstat output to stdout.";

    stdout_plugin.handle_output = &dstat_handle_output_stdout;
    stdout_plugn.id = plugin_id;

    plugin_ctx->registered_plugins[plugin_id] = &stdout_plugin;
    plugin_ctx->plugin_count++;
}

/* EOF */

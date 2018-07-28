#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <glib.h>

#include "defines.h"
#include "config.h"
#include "chat.h"

/* Acceptable command line options */
char *config_path;

GOptionEntry options[] = {
    { "config", 'c', 0,
      G_OPTION_ARG_STRING, &config_path, "Configuration file", NULL }
};

int
main (int argc, char *argv[]) {
    Config *config;
    GError *error;
    GMainLoop *loop = g_main_loop_new (NULL, FALSE);
    GOptionContext *context;

    error = NULL;

#ifndef _WIN32
    /* libpurple's built-in DNS resolution forks processes to perform
     * blocking lookups without blocking the main process.  It does not
     * handle SIGCHLD itself, so if the UI does not you quickly get an army
     * of zombie subprocesses marching around.
     */
    signal (SIGCHLD, SIG_IGN);
#endif

    /* Parse options */
    context = g_option_context_new ("- a helpful xmpp bot");
    g_option_context_add_main_entries (context, options, NULL);
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_warning ("Error parsing arguments: %s\n", error->message);
        g_error_free (error);
        error = NULL;
    }

    if (NULL == config_path) {
        config_path = DEFAULT_CONFIG_PATH;
    }

    config = get_config (config_path, &error);
    if (NULL == config) {
        g_error ("Error: cannot find configuration file.\n");
    }

    initialize_libpurple (config);
    g_main_loop_run (loop);
    purple_plugins_save_loaded (PLUGIN_SAVE_PREF);

    return 0;
}

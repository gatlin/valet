#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <glib.h>

#include "defines.h"
#include "config.h"
#include "chat.h"


int
main (int argc, char *argv[]) {
    Config *config;
    GMainLoop *loop = g_main_loop_new (NULL, FALSE);

#ifndef _WIN32
    /* libpurple's built-in DNS resolution forks processes to perform
     * blocking lookups without blocking the main process.  It does not
     * handle SIGCHLD itself, so if the UI does not you quickly get an army
     * of zombie subprocesses marching around.
     */
    signal (SIGCHLD, SIG_IGN);
#endif

    config = get_config (NULL);
    if (NULL == config) {
        fprintf (stderr, "Error: cannot find configuration file.\n");
        return -1;
    }

    initialize_libpurple (config);
    g_main_loop_run (loop);
    purple_plugins_save_loaded (PLUGIN_SAVE_PREF);

    return 0;
}

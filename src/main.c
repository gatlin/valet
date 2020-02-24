#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <adapters/glib.h>

#include "defines.h"
#include "context.h"
#include "chat.h"

/* Global values! */
char *config_path;
GMainLoop *loop;

static void
handle_term (int signum) {
  if (g_main_loop_is_running (loop)) {
    g_message ("Caught SIGTERM ...");
    g_main_loop_quit (loop);
  }
}

static void
redis_connect_cb (const redisAsyncContext *ac G_GNUC_UNUSED,
                  int status) {
  if (REDIS_OK != status) {
    g_debug ("Failed to connect to redis: %s", ac->errstr);
  }
  else {
    g_message ("Redis connected");
  }
}

/* Acceptable command line options */
GOptionEntry options[] = {
    { "config", 'c', 0,
      G_OPTION_ARG_STRING, &config_path,
      "Location of configuration file", NULL }
};

int
main (int argc, char *argv[]) {
  GMainContext *gmc = NULL;
  Context *valet_context;
  GError *error;
  GOptionContext *context;
  GSource *source = NULL;
  struct sigaction action;

  loop = g_main_loop_new (gmc, FALSE);
  memset (&action, 0, sizeof (struct sigaction));
  action.sa_handler = handle_term;
  sigaction (SIGINT, &action, NULL);

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

  valet_context = get_context (config_path, &error);
  if (NULL == valet_context) {
    g_error ("Error: cannot find configuration file.\n");
  }

  if (NULL != valet_context->redisCtx) {
    source = redis_source_new (valet_context->redisCtx);
    redisAsyncSetConnectCallback (valet_context->redisCtx, redis_connect_cb);
    g_source_attach (source, gmc);
  }

  initialize_libpurple (valet_context);

  g_main_loop_run (loop);
  purple_plugins_save_loaded (PLUGIN_SAVE_PREF);

  return 0;
}

/***
 * config.c
 * This code is responsible for acquiring any and all configuration data from
 * the environment.
 */

#include "context.h"
#include "defines.h"

Context *
get_context (char *config_path, GError **caller_error) {
    Context *context;
    GKeyFile *keyfile;
    GKeyFileFlags flags;
    GError *error;

    error = NULL;

    keyfile = g_key_file_new ();
    flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

    if (!g_key_file_load_from_file (keyfile,
                                    config_path,
                                    flags,
                                    &error)) {
        g_propagate_error (caller_error, error);
        return NULL;
    }

    context = g_slice_new (Context);
    context->username = g_key_file_get_string (keyfile, "credentials",
                                              "username", NULL);
    context->password = g_key_file_get_string (keyfile, "credentials",
                                              "password", NULL);

    context->purple_data = g_key_file_get_string (keyfile, "valet",
                                                 "libpurpledata", NULL);
    context->lurch_path = g_key_file_get_string (keyfile, "valet",
                                                "lurch", NULL);
    context->commands_path = g_key_file_get_string (keyfile, "valet",
                                                   "commands", NULL);
    g_key_file_free (keyfile);
    return context;
}

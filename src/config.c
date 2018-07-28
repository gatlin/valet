/***
 * config.c
 * This code is responsible for acquiring any and all configuration data from
 * the environment.
 */

#include "config.h"
#include "defines.h"

Config *
get_config (char *config_path, GError **caller_error) {
    Config *config;
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

    config = g_slice_new (Config);
    config->username = g_key_file_get_string (keyfile, "credentials",
                                              "username", NULL);
    config->password = g_key_file_get_string (keyfile, "credentials",
                                              "password", NULL);

    config->purple_data = g_key_file_get_string (keyfile, "valet",
                                                 "libpurpledata", NULL);
    config->lurch_path = g_key_file_get_string (keyfile, "valet",
                                                "lurch", NULL);
    config->commands_path = g_key_file_get_string (keyfile, "valet",
                                                   "commands", NULL);
    g_key_file_free (keyfile);
    return config;
}

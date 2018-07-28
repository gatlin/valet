/***
 * config.c
 * This code is responsible for acquiring any and all configuration data from
 * the environment.
 */

#include "config.h"
#include "defines.h"

Credentials *
get_account_credentials () {
    Credentials *credentials;
    GKeyFile *keyfile;
    GKeyFileFlags flags;
    GError *error = NULL;

    keyfile = g_key_file_new ();
    flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

    if (!g_key_file_load_from_file (keyfile,
                                    CONFIG_PATH,
                                    flags,
                                    &error)) {
        g_error("%s\n",error->message);
        g_error_free (error);
        return NULL;
    }

    credentials = g_slice_new (Credentials);
    credentials->username = g_key_file_get_string (keyfile, "credentials",
                                                   "username", NULL);
    credentials->password = g_key_file_get_string (keyfile, "credentials",
                                                   "password", NULL);
    g_key_file_free (keyfile);
    return credentials;
}

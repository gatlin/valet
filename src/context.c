/***
 * context.c
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

    context->bonjour_enabled = g_key_file_get_boolean (keyfile, "valet",
                                                       "bonjour", NULL);
    context->purple_data = g_key_file_get_string (keyfile, "valet",
                                                 "libpurpledata", NULL);
    context->lurch_path = g_key_file_get_string (keyfile, "valet",
                                                "lurch", NULL);
    context->commands_path = g_key_file_get_string (keyfile, "valet",
                                                   "commands", NULL);
    context->kvstore = g_hash_table_new (g_str_hash, g_str_equal);

    if (g_key_file_has_group (keyfile, "redis")) {
      gchar *redis_host = g_key_file_get_string (keyfile, "redis", "host", NULL);
      gint redis_port = g_key_file_get_integer (keyfile, "redis", "port", NULL);
      g_debug ("redis group exists: %s : %d", redis_host, redis_port);
      context->redisCtx = redisAsyncConnect (redis_host, redis_port);
      if (context->redisCtx->err) {
        g_debug ("redis error");
        g_printerr ("redis error: %s\n",context->redisCtx->errstr);
        context->redisCtx = NULL;
      }
    }
    else {
      context->redisCtx = NULL;
    }

    g_key_file_free (keyfile);
    return context;
}

gboolean
valet_set_key (Context *context, const gchar *key, const gchar *value ) {
    gboolean ret = FALSE;

    if (g_hash_table_contains(context->kvstore, key)) {
        ret = g_hash_table_replace (context->kvstore,
                                    g_strdup (key),
                                    g_strdup (value));
    }
    else {
        ret = g_hash_table_insert (context->kvstore,
                                   g_strdup (key),
                                   g_strdup (value));
    }
    return ret;
}

gchar *
valet_get_key (Context *context, const gchar *key) {
    return g_hash_table_lookup (context->kvstore, key);
}

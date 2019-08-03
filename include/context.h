#ifndef __VALET_CONTEXT_H
#define __VALET_CONTEXT_H

#include <hiredis.h>
#include <async.h>
#include <glib.h>

#include <gmodule.h>

/**
 * A Context is essentially global data for the program.
 *
 * In addition to containing execution-specific configuration data, Context
 * contains a key-value store that can be read from and written to by the user
 * and also queried by spawned processes.
 *
 * This allows sensitive information such as passwords to be stored in memory
 * and updated during program execution by the user.
 */
typedef struct {
  char *username;
  char *password;
  char *purple_data; /* Path to store purple account data */
  char *lurch_path; /* Location of the lurch.so file */
  char *commands_path; /* Path where commands are located */
  gboolean bonjour_enabled;
  GHashTable *kvstore;
  redisAsyncContext *redisCtx;
} Context;

Context *get_context (char *, GError **);
gboolean valet_set_key (Context *, const gchar *, const gchar *);
gboolean valet_get_key (Context *, const gchar *, gpointer);

#endif /* __VALET_CONTEXT_H */

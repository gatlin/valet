#ifndef __FIGARO_CONFIG_H
#define __FIGARO_CONFIG_H

#include <glib.h>

typedef struct {
    gchar *username;
    gchar *password;
} Credentials;

Credentials *get_account_credentials ();

#endif /* __FIGARO_CONFIG_H */

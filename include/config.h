#ifndef __VALET_CONFIG_H
#define __VALET_CONFIG_H

#include <glib.h>

typedef struct {
    gchar *username;
    gchar *password;
} Credentials;

Credentials *get_account_credentials ();

#endif /* __VALET_CONFIG_H */

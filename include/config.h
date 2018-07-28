#ifndef __VALET_CONFIG_H
#define __VALET_CONFIG_H

#include <glib.h>

typedef struct {
    char *username;
    char *password;
    char *purple_data; /* Path to store purple account data */
    char *lurch_path; /* Location of the lurch.so file */
    char *commands_path; /* Path where commands are located */
} Config;

Config *get_config (char *, GError **);

#endif /* __VALET_CONFIG_H */

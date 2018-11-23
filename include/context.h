#ifndef __VALET_CONTEXT_H
#define __VALET_CONTEXT_H

#include "uthash.h"
#include <glib.h>

typedef struct {
    char *username;
    char *password;
    char *purple_data; /* Path to store purple account data */
    char *lurch_path; /* Location of the lurch.so file */
    char *commands_path; /* Path where commands are located */

} Context;

Context *get_context (char *, GError **);

#endif /* __VALET_CONTEXT_H */

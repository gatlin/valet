#ifndef __FIGARO_CHAT_H
#define __FIGARO_CHAT_H

#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include "win32/win32dep.h"
#endif

#include "purple.h"
#include <glib.h>

#include "config.h"
#include "defines.h"

void
initialize_libpurple (Credentials *);

#endif /* __FIGARO_CHAT_H */

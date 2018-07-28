#ifndef __FIGARO_RESPONSE_H
#define __FIGARO_RESPONSE_H

#include "defines.h"
#include "purple.h"
#include <glib.h>

void
received_im(PurpleAccount *, char *, char *, PurpleConversation *,
            PurpleMessageFlags, void *);

#endif /* __FIGARO_RESPONSE_H */

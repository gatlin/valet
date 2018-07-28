#ifndef __VALET_RESPONSE_H
#define __VALET_RESPONSE_H

#include "defines.h"
#include "purple.h"
#include <glib.h>

void
received_im(PurpleAccount *, char *, char *, PurpleConversation *,
            PurpleMessageFlags, void *);

#endif /* __VALET_RESPONSE_H */

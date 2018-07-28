/***
 * response.c
 * This code is responsible for spawning the appropriate commands and replying
 * with the output.
 */
#include "response.h"

/**
 * The arguments, output file descriptors, and libpurple conversation comprising
 * a given command.
 */
typedef struct {
    char **args;
    int child_stdout;
    int child_stderr;
    PurpleConvIm *im;
    GPid pid;
    guint watch_id;
} Command;

Command *
figaro_command_new (char *args, PurpleConvIm *im) {
    Command *command;
    command = g_new0 (Command, 1);
    command->args = g_regex_split_simple("[\\s+]", args, 0, 0);
    command->child_stdout = -1;
    command->child_stderr = -1;
    command->im = im;
    command->pid = -1;
    command->watch_id = 0; /* unsigned */
    return command;
}

void
figaro_command_free (Command *command) {
    if (NULL != command->args) {
        g_strfreev (command->args);
    }
    g_spawn_close_pid (command->pid);
    g_free (command);
}

/**
 * Called by the GLib event loop whenever a command produces output.
 */
static gboolean
reply (GIOChannel *channel, GIOCondition cond, gpointer data) {
    GIOStatus status;
    GError *error;
    Command *command;
    char *buffer;
    gsize length, term_pos;
    gboolean more_data;

    more_data = TRUE;
    error = NULL;
    command = data;
    status = g_io_channel_read_line (channel, &buffer, &length, &term_pos,
                                     &error);

    if (NULL != error) {
        fprintf (stderr, "Error: %s\n", error->message);
        g_error_free (error);
    }

    if (G_IO_STATUS_NORMAL == status) {
        /* Strip the trailing newline */
        buffer[strcspn (buffer, "\n")] = 0;
        purple_conv_im_send (command->im, buffer);
        free (buffer);
    }

    else { /* ERROR, EOF */
        more_data = FALSE;
        g_io_channel_shutdown (channel, TRUE, NULL);
        figaro_command_free (command);
    }

    return more_data;
}

/**
 * Set up response channels from child process output.
 */
void
create_response_channels (Command *command) {
    GIOChannel *out_channel;

    out_channel = g_io_channel_unix_new (command->child_stdout);

    g_io_add_watch_full (out_channel, G_PRIORITY_DEFAULT,
                         G_IO_IN | G_IO_HUP, reply,
                         command, NULL);
}

/**
 * Parse an incoming message and try to spawn a command.
 * Commands are defined as CMD_PATH in defines.h
 */
void
spawn_command (char *buffer, PurpleConvIm *im) {
    GError *error = NULL;
    Command *command;

    command = figaro_command_new (buffer, im);

    /* Spawn a new process */
    g_spawn_async_with_pipes (CMD_PATH,
                              command->args,
                              NULL, G_SPAWN_DEFAULT,
                              NULL, NULL,
                              &(command->pid), NULL,
                              &(command->child_stdout),
                              &(command->child_stderr),
                              &error);

    /* Did GLib tell us something went wrong? */
    if (NULL != error) {
        fprintf (stderr, "Spawning child failed: %s\n", error->message);
        figaro_command_free (command);
        g_error_free (error);
        return;
    }

    /* Did GLib lie? */
    if (-1 == command->child_stdout || -1 == command->child_stderr) {
        fprintf (stderr, "Error capturing command output.\n");
        figaro_command_free (command);
        return;
    }

    /* Okay we've started a process let's do it. */
    create_response_channels (command);
}

/**
 * The first message received has a null conversation. This is used to ensure a
 * conversation exists in libpurple so that we may respond.
 */
static PurpleConversation *
ensure_conversation (PurpleConversation *conv,
                     PurpleAccount *account,
                     char *sender) {
    if (NULL == conv) {
        conv = purple_conversation_new (PURPLE_CONV_TYPE_IM,
                                        account,
                                        sender);
    }
    return conv;
}

/**
 * This function is subscribed to the "received-im-msg" libpurple signal.
 * It first checks that the message comes from a contact on the buddy list
 * before spawning the command.
 */
void
received_im(PurpleAccount *account, char *sender, char *buffer,
            PurpleConversation *conv, PurpleMessageFlags flags,
            void *data)
{
    PurpleBuddy *buddy;
    PurpleConvIm *im;

    conv = ensure_conversation (conv, account, sender);
    im = purple_conversation_get_im_data (conv);
    if (NULL == im) {
        return;
    }

    buddy = purple_find_buddy (account, sender);
    if (NULL == buddy) {
        printf ("Received message from unknown sender: %s\n", sender);
        return;
    }

    spawn_command (buffer, im);
}

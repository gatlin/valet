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
} Command;

Command *
figaro_command_new (char *args, PurpleConvIm *im) {
    Command *command;
    command = g_new0 (Command, 1);
    command->args = g_regex_split_simple("[\\s+]", args, 0, 0);
    command->child_stdout = -1;
    command->child_stderr = -1;
    command->im = im;
    return command;
}

void
figaro_command_free (Command *command) {
    if (NULL != command->args) {
        g_strfreev (command->args);
    }
    g_free (command);
}

/**
 * Called by the GLib event loop whenever a command produces output.
 */
static gboolean
reply (GIOChannel *channel, GIOCondition cond, gpointer data) {
    GIOStatus status;
    GError *error;
    PurpleConvIm *im;
    char *buffer;
    gsize length, term_pos;
    gboolean more_data;

    more_data = TRUE;
    error = NULL;
    im = data;
    status = g_io_channel_read_line (channel, &buffer, &length, &term_pos,
                                     &error);

    if (NULL != error) {
        fprintf (stderr, "Error: %s\n", error->message);
        g_error_free (error);
    }
    switch (status) {
    case G_IO_STATUS_NORMAL:
        buffer[strcspn(buffer, "\n")] = 0;
        purple_conv_im_send (im, buffer);
        free (buffer);
        break;

    case G_IO_STATUS_EOF:
        more_data = FALSE;
        break;
    default:
        ;
    }

    return more_data;
}

/**
 * Set up response channels from child process output.
 */
void
create_response_channels (Command *command) {
    GIOChannel *out_channel, *err_channel;

    out_channel = g_io_channel_unix_new (command->child_stdout);
    err_channel = g_io_channel_unix_new (command->child_stderr);

    g_io_add_watch_full (out_channel, G_PRIORITY_DEFAULT, G_IO_IN, reply,
                         command->im, NULL);
    g_io_add_watch_full (err_channel, G_PRIORITY_DEFAULT, G_IO_IN, reply,
                         command->im, NULL);

    g_io_channel_unref (out_channel);
    g_io_channel_unref (err_channel);
    figaro_command_free (command);
}

/**
 * Parse an incoming message and try to spawn a command.
 * Commands are defined as CMD_PATH in defines.h
 */
void
spawn_command (Command *command) {
    GPid child_pid;
    GError *error = NULL;

    g_spawn_async_with_pipes (CMD_PATH,
                              command->args,
                              NULL, G_SPAWN_DEFAULT,
                              NULL, NULL, &child_pid, NULL,
                              &(command->child_stdout),
                              &(command->child_stderr),
                              &error);

    if (NULL != error) {
        fprintf (stderr, "Spawning child failed: %s\n", error->message);
        figaro_command_free (command);
        g_error_free (error);
        return;
    }

    if (-1 == command->child_stdout || -1 == command->child_stderr) {
        fprintf (stderr, "Error capturing command output.\n");
        figaro_command_free (command);
        return;
    }

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
    Command *command;
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

    command = figaro_command_new (buffer, im);
    spawn_command (command);
}

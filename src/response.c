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
} Command;

Command *
valet_command_new (char *args, PurpleConvIm *im) {
    Command *command;
    command = g_new0 (Command, 1);
    command->args = g_regex_split_simple("[\\s+]", args, 0, 0);
    command->child_stdout = -1;
    command->child_stderr = -1;
    command->im = im;
    command->pid = -1;
    return command;
}

void
valet_command_free (Command *command) {
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
        g_warning ("Error: %s\n", error->message);
        g_error_free (error);
    }

    if (G_IO_STATUS_NORMAL == status) {
        /* Strip the trailing newline */
        buffer[strcspn (buffer, "\n")] = 0;
        purple_conv_im_send (im, buffer);
        free (buffer);
    }

    else { /* ERROR, EOF */
        more_data = FALSE;
        g_io_channel_shutdown (channel, TRUE, NULL);
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

    g_io_add_watch_full (out_channel, G_PRIORITY_DEFAULT,
                         G_IO_IN | G_IO_HUP, reply,
                         command->im, NULL);

    g_io_add_watch_full (err_channel, G_PRIORITY_DEFAULT,
                         G_IO_IN | G_IO_HUP, reply,
                         command->im, NULL);
}

/**
 * Called when the command process completes.
 */
void
command_process_watch (GPid pid, int status, gpointer data) {
    g_debug ("Command process %" G_PID_FORMAT " exited %s\n", pid,
             g_spawn_check_exit_status (status, NULL)
             ? "normally" : "abnormally");
    g_spawn_close_pid (pid);
}

/**
 * Parse an incoming message and try to spawn a command.
 * Commands are defined as CMD_PATH in defines.h
 */
void
spawn_command (char *buffer, PurpleConvIm *im, char *commands_path) {
    Command *command;
    GError *error;

    error = NULL;
    command = valet_command_new (buffer, im);

    /* Spawn a new process */
    g_spawn_async_with_pipes (commands_path,
                              command->args,
                              NULL,
                              //G_SPAWN_DEFAULT,
                              G_SPAWN_DO_NOT_REAP_CHILD,
                              NULL, NULL,
                              &(command->pid), NULL,
                              &(command->child_stdout),
                              &(command->child_stderr),
                              &error);

    /* Did GLib tell us something went wrong? */
    if (NULL != error) {
        g_warning ("Spawning child failed: %s\n", error->message);
        purple_conv_im_send (command->im, "I do not recognize this command.");
        g_error_free (error);
        valet_command_free (command);
        return;
    }

    /* Did GLib lie? */
    if (-1 == command->child_stdout || -1 == command->child_stderr) {
        g_warning ("Error capturing child process output.\n");
        valet_command_free (command);
        return;
    }

    /* Okay we've started a process let's do it. */
    create_response_channels (command);
    g_child_watch_add (command->pid, command_process_watch, NULL);

    /* Cool */
    valet_command_free (command);
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
received_im (PurpleAccount *account, char *sender, char *buffer,
             PurpleConversation *conv, PurpleMessageFlags flags,
             void *data)
{
    PurpleBuddy *buddy;
    PurpleConvIm *im;
    char *commands_path;

    commands_path = data;
    conv = ensure_conversation (conv, account, sender);
    im = purple_conversation_get_im_data (conv);
    if (NULL == im) {
        return;
    }

    buddy = purple_find_buddy (account, sender);
    if (NULL == buddy) {
        g_message ("Received message from unknown sender: %s\n", sender);
        return;
    }

    spawn_command (buffer, im, commands_path);
}

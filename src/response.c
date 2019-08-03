/***
 * response.c
 * This code is responsible for spawning the appropriate commands and replying
 * with the output.
 */
#include "response.h"
#include "context.h"

/**
 * The arguments, output file descriptors, and libpurple conversation comprising
 * a given command.
 */
typedef struct {
  char **args;
  int child_stdin;
  int child_stdout;
  int child_stderr;
  PurpleConvIm *im;
  GPid pid;
  Context *context;
} Command;

Command *
valet_command_new (char *args, PurpleConvIm *im, Context *context) {
  Command *command;
  char *tmp = NULL;
  command = g_new0 (Command, 1);

  if (g_str_has_prefix (args, "<font>")) {
    /* This is likely a Bonjour message with hardcoded formatting. */
    GMatchInfo *match_info;
    GRegex *regex =  g_regex_new ("^<font>(.*)</font>", 0, 0, NULL);
    tmp = args;
    g_regex_match (regex, tmp, 0, &match_info);
    args = g_match_info_fetch (match_info, 1);
    g_free (match_info);
    g_regex_unref (regex);
  }

  command->args = g_regex_split_simple("[\\s+]", args, 0, 0);
  command->child_stdin = -1;
  command->child_stdout = -1;
  command->child_stderr = -1;
  command->im = im;
  command->pid = -1;
  command->context = context;

  if (NULL != tmp) {
    g_free (tmp);
    tmp = NULL;
  }
  return command;
}

void
valet_command_free (Command *command) {
  if (NULL != command->args) {
    g_strfreev (command->args);
  }
  g_free (command);
}

/* Helper function which writes a string to a file descriptor.
   Keeps `reply` less cluttered.
*/
static gboolean
write_to_fd (int fd, const gchar *message) {
  gsize bw;
  GIOChannel *channel = g_io_channel_unix_new (fd);
  GString *message_str = g_string_new (message);
  g_string_append (message_str, "\n");
  g_io_channel_write_chars (channel,
                            message_str->str,
                            message_str->len,
                            &bw,
                            NULL);
  g_string_free (message_str, TRUE);
  g_io_channel_shutdown (channel, TRUE, NULL);
  g_io_channel_unref (channel);
  return TRUE;
}

static gboolean
handle_set_key (Context *context, PurpleConvIm *im, char *str) {
  if (!g_str_has_prefix (str, "#set")) {
    return FALSE;
  }
  GMatchInfo *match_info;
  GRegex *regex = g_regex_new ("^#set\\s+(\\S+)\\s+(.*)$", 0, 0, NULL);

  g_regex_match (regex, str, 0, &match_info);

  gchar *key = g_match_info_fetch (match_info, 1);
  gchar *val = g_match_info_fetch (match_info, 2);

  valet_set_key (context, key, val);
  purple_conv_im_send (im, "Inserted key value pair.");

  g_free (key);
  g_free (val);
  g_free (match_info);
  g_regex_unref (regex);
  return TRUE;
}

static gboolean
handle_get_key (Context *context, PurpleConvIm *im, char *str) {
  if (!g_str_has_prefix (str, "#get")) {
    return FALSE;
  }
  GMatchInfo *match_info;
  GRegex *regex = g_regex_new ("^#get\\s+(\\S+)", 0, 0, NULL);

  g_regex_match (regex, str, 0, &match_info);

  gchar *key = g_match_info_fetch (match_info, 1);

  gchar *val = valet_get_key (context, key);
  if (NULL == val) {
    purple_conv_im_send (im, "No value found for key.");
  }
  else {
    purple_conv_im_send (im, val);
    g_free (val);
  }

  g_free (key);
  g_free (match_info);
  g_regex_unref (regex);
  return TRUE;
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
  char **split_response;
  Command *command = data;

  more_data = TRUE;
  error = NULL;
  im = command->im;
  status = g_io_channel_read_line (channel, &buffer, &length, &term_pos,
                                   &error);

  if (NULL != error) {
    g_warning ("Error: %s\n", error->message);
    g_error_free (error);
  }

  if (G_IO_STATUS_NORMAL == status) {
    /* Strip the trailing newline */
    buffer[strcspn (buffer, "\n")] = 0;
    /* If the command needs some more info, reply with it here. */
    if (g_str_has_prefix (buffer, "#get")) {
      /* Split the response and treat the second part as a lookup key. */
      split_response = g_regex_split_simple ("[\\s+]", buffer, 0, 0);
      gchar *val = valet_get_key (command->context, split_response[1]);

      if (NULL == val) {
        // failure
        GString *err_str = g_string_new ("Undefined key: " );
        g_string_append (err_str, split_response[1]);
        purple_conv_im_send (im, err_str->str);
        g_string_free (err_str, TRUE);
        more_data = FALSE;
      }
      else {
        write_to_fd (command->child_stdin, val);
      }
    }
    else {
      purple_conv_im_send (im, buffer);
    }

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
                       command, NULL);

  g_io_add_watch_full (err_channel, G_PRIORITY_DEFAULT,
                       G_IO_IN | G_IO_HUP, reply,
                       command, NULL);
}

/**
 * Called when the command process completes.
 */
void
command_process_watch (GPid pid, int status, gpointer data) {
  g_debug ("Command process %d exited %s\n", pid,
           g_spawn_check_exit_status (status, NULL)
           ? "normally" : "abnormally");
  g_spawn_close_pid (pid);
  valet_command_free (data);
}

/**
 * Parse an incoming message and try to spawn a command.
 * Commands are defined as CMD_PATH in defines.h
 */
void
spawn_command (char *buffer, PurpleConvIm *im, Context *context) {
  Command *command;
  GError *error;
  char *commands_path;

  error = NULL;
  commands_path = context->commands_path;
  command = valet_command_new (buffer, im, context);

  /* Spawn a new process */
  g_spawn_async_with_pipes (commands_path,
                            command->args,
                            NULL,
                            //G_SPAWN_DEFAULT,
                            G_SPAWN_DO_NOT_REAP_CHILD,
                            NULL, NULL,
                            &(command->pid),
                            &(command->child_stdin),
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
  g_child_watch_add (command->pid, command_process_watch, command);
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
             void *data) {
  PurpleBuddy *buddy;
  PurpleConvIm *im;
  Context *context;

  context = data;
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

  if (handle_set_key (context, im, buffer)) {
    return;
  }

  else if (handle_get_key (context, im, buffer)) {
    return;
  }

  else {
    spawn_command (buffer, im, context);
  }
}

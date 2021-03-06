/***
 * chat.c
 * This is where the libpurple boilerplate is defined.
 * Messages are routed from here to `received_im` in response.c
 */

#include "chat.h"
#include "response.h"

/*** The first part of this code stolen shamelessly from the libpurple example
     `nullclient` ***/

#define PURPLE_GLIB_READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define PURPLE_GLIB_WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

typedef struct _PurpleGLibIOClosure {
  PurpleInputFunction function;
  guint result;
  gpointer data;
} PurpleGLibIOClosure;

static void
purple_glib_io_destroy(gpointer data) {
  g_free (data);
}

static gboolean
purple_glib_io_invoke (GIOChannel *source,
                       GIOCondition condition,
                       gpointer data) {
  PurpleGLibIOClosure *closure = data;
  PurpleInputCondition purple_cond = 0;

  if (condition & PURPLE_GLIB_READ_COND) {
    purple_cond |= PURPLE_INPUT_READ;
  }
  if (condition & PURPLE_GLIB_WRITE_COND) {
    purple_cond |= PURPLE_INPUT_WRITE;
  }

  closure->function
    (closure->data, g_io_channel_unix_get_fd(source), purple_cond);

  return TRUE;
}

static guint
glib_input_add (gint fd, PurpleInputCondition condition,
                PurpleInputFunction function, gpointer data) {
  PurpleGLibIOClosure *closure = g_new0 (PurpleGLibIOClosure, 1);
  GIOChannel *channel;
  GIOCondition cond = 0;

  closure->function = function;
  closure->data = data;

  if (condition & PURPLE_INPUT_READ) {
    cond |= PURPLE_GLIB_READ_COND;
  }
  if (condition & PURPLE_INPUT_WRITE) {
    cond |= PURPLE_GLIB_WRITE_COND;
  }

#if defined _WIN32 && !defined WINPIDGIN_USE_GLIB_IO_CHANNEL
  channel = wpurple_g_io_channel_win32_new_socket (fd);
#else
  channel = g_io_channel_unix_new (fd);
#endif
  closure->result = g_io_add_watch_full
    (channel, G_PRIORITY_DEFAULT, cond,
     purple_glib_io_invoke, closure,
     purple_glib_io_destroy);

  g_io_channel_unref (channel);
  return closure->result;
}

static PurpleEventLoopUiOps glib_eventloops =
  { g_timeout_add,
    g_source_remove,
    glib_input_add,
    g_source_remove,
    NULL,
#if GLIB_CHECK_VERSION(2,14,0)
    g_timeout_add_seconds,
#else
    NULL,
#endif

    /* padding */
    NULL,
    NULL,
    NULL };

/*** End of the eventloop functions. ***/

/**
 * Callback which receives messages and logs them.
 */
static void
valet_write_conv (PurpleConversation *conv,
                  const char *who,
                  const char *alias,
                  const char *message,
                  PurpleMessageFlags flags,
                  time_t mtime) {
  const char *name;
  if (alias && *alias) {
    name = alias;
  }
  else if (who && *who) {
    name = who;
  }
  else {
    name = NULL;
  }

  g_debug ("(%s) %s %s: %s\n", purple_conversation_get_name (conv),
           purple_utf8_strftime ("(%H:%M:%S)", localtime (&mtime)),
           name, message);
}

static PurpleConversationUiOps valet_conv_uiops =
  { NULL,                      /* create_conversation  */
    NULL,                      /* destroy_conversation */
    NULL,                      /* write_chat           */
    NULL,                      /* write_im             */
    valet_write_conv,           /* write_conv           */
    NULL,                      /* chat_add_users       */
    NULL,                      /* chat_rename_user     */
    NULL,                      /* chat_remove_users    */
    NULL,                      /* chat_update_user     */
    NULL,                      /* present              */
    NULL,                      /* has_focus            */
    NULL,                      /* custom_smiley_add    */
    NULL,                      /* custom_smiley_write  */
    NULL,                      /* custom_smiley_close  */
    NULL,                      /* send_confirm         */
    NULL,
    NULL,
    NULL,
    NULL };

/**
 * Give libpurple hooks into our (paltry) UI.
 */
static void
valet_ui_init (void) {
  purple_conversations_set_ui_ops (&valet_conv_uiops);
}

static PurpleCoreUiOps null_core_uiops =
  { NULL,
    NULL,
    valet_ui_init,
    NULL,

    /* padding */
    NULL,
    NULL,
    NULL,
    NULL };

static void
init_libpurple (char *purple_data_path) {
  /* Set a custom user directory (optional) */
  purple_util_set_user_dir (purple_data_path);

  /* We do not want any debugging for now to keep the noise to a minimum. */
  purple_debug_set_enabled (FALSE);

  /* Set the core-uiops, which is used to
   *  - initialize the ui specific preferences.
   *  - initialize the debug ui.
   *  - initialize the ui components for all the modules.
   *  - uninitialize the ui components for all the modules when the core terminates.
   */
  purple_core_set_ui_ops (&null_core_uiops);

  /* Set the uiops for the eventloop. If your client is glib-based, you can safely
   * copy this verbatim. */
  purple_eventloop_set_ui_ops (&glib_eventloops);

  /* Set path to search for plugins. The core (libpurple) takes care of loading the
   * core-plugins, which includes the protocol-plugins. So it is not essential to add
   * any path here, but it might be desired, especially for ui-specific plugins. */
  /* purple_plugins_add_search_path (CUSTOM_PLUGIN_PATH); */

  /* Now that all the essential stuff has been set, let's try to init the core. It's
   * necessary to provide a non-NULL name for the current ui to the core. This name
   * is used by stuff that depends on this ui, for example the ui-specific plugins. */
  if (!purple_core_init (UI_ID)) {
    /* Initializing the core failed. Terminate. */
    g_error ("libpurple initialization failed. Dumping core.\n"
             "Please report this!\n");
    /* Goodbye! */
  }

  /* Create and load the buddylist. */
  purple_set_blist (purple_blist_new ());
  purple_blist_load ();

  /* Load the preferences. */
  purple_prefs_init ();
  purple_prefs_load ();

  /* Load the desired plugins. The client should save the list of loaded plugins in
   * the preferences using purple_plugins_save_loaded(PLUGIN_SAVE_PREF) */
  purple_plugins_load_saved (PLUGIN_SAVE_PREF);

  /* Load the pounces. */
  purple_pounces_load ();
}

/**
 * This function is subscribed to the "signed-on" libpurple signal.
 */
static void
signed_on(PurpleConnection *gc, gpointer null) {
  PurpleAccount *account = purple_connection_get_account (gc);
  g_message ("Account connected: %s %s",
             account->username, account->protocol_id);
}

static void
connect_to_signals (Context *context) {
  static int signed_on_handle;
  static int received_im_msg_handle;

  /*    static int conversation_created_handle; */
  purple_signal_connect (purple_connections_get_handle (),
                         "signed-on", &signed_on_handle,
                         PURPLE_CALLBACK(signed_on), NULL);

  purple_signal_connect (purple_conversations_get_handle (),
                         "received-im-msg", &received_im_msg_handle,
                         PURPLE_CALLBACK(received_im), context);
}

/**
 * Initializes the `lurch` OMEMO plugin.
 */
void
initialize_omemo (char *lurch_path) {
  PurplePlugin *lurch = purple_plugin_probe (lurch_path);

  if (lurch != NULL) {
    if (purple_plugin_load (lurch)) {
      g_message ("Successfully loaded OMEMO support.");
    }
    else {
      g_message ("Error initializing OMEMO support.");
    }
  }
  else {
    g_warning ("Error: could not load OMEMO plugin.");
  }
}

/**
 * Authenticate with the server and begin listening for messages.
 */
void
initialize_libpurple (Context *context) {
  PurpleAccount *xmpp_account, *bonjour_account;
  PurpleSavedStatus *status;

  init_libpurple (context->purple_data);

  if (NULL != context->username) {
    /* Authenticate with the server */
    xmpp_account = purple_account_new (context->username, "prpl-jabber");
    purple_account_set_password (xmpp_account, context->password);
    purple_account_set_enabled (xmpp_account, UI_ID, TRUE);
  }

  if (context->bonjour_enabled) {
    /* Also set up a bonjour user because why not */
    bonjour_account = purple_account_new ("exodus", "prpl-bonjour");
    purple_account_set_alias (bonjour_account, context->username);
    purple_account_set_enabled (bonjour_account, UI_ID, TRUE);
  }

  /* Set our status */
  status = purple_savedstatus_new (NULL, PURPLE_STATUS_AVAILABLE);
  purple_savedstatus_activate (status);

  connect_to_signals (context);

  initialize_omemo (context->lurch_path);
}

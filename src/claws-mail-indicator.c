/*
 *  Authors:
 *    Marek Habersack <grendel@twistedcode.net>
 *
 *  (C) 2011 Marek Habersack
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted
 *  provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of
 *       conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name of Marek Habersack nor the names of its contributors may be used to
 *       endorse or promote products derived from this software without specific prior written
 *       permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <common/claws.h>
#include <common/utils.h>
#include <plugin.h>
#include <account.h>
#include <mainwindow.h>
#include <toolbar.h>
#include <addressbook.h>
#include <compose.h>
#include <inc.h>
#include <main.h>
#include <folder.h>

/* Workaround claws-mail including config.h from public headers */
#undef GETTEXT_PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#ifdef HAVE_CONFIG_H
#include <indicator_config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#ifdef HAVE_LIBCANBERRA
#include <canberra.h>
#endif

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#ifdef HAVE_UNITY
#include <unity.h>
#endif

#include <libindicate/server.h>
#include <libindicate/indicator.h>
#include <libindicate/indicator-messages.h>

#undef HAVE_CONFIG_H
#include <folder.h>

#include "claws-mail-version.h"

#define CLAWS_DESKTOP_FILE       "/usr/share/applications/claws-mail.desktop"
#define USER_BLACKLIST_DIR       "indicators/messages/applications-blacklist"
#define USER_BLACKLIST_FILENAME "claws-mail"
#define APPLICATION_NAME        "claws-mail-indicator"
#define APPLICATION_ID          "org.grendel." APPLICATION_NAME

#define INDICATOR_NEW_COUNT_NAME "New messages"

static guint item_hook_id;
static IndicateServer *indicate_server = NULL;
static GHashTable *indicators = NULL;

#ifdef HAVE_UNITY
static UnityLauncherEntry *unity_launcher = NULL;
#endif
	
#ifdef HAVE_LIBNOTIFY
static NotifyNotification *notification = NULL;
#endif

#ifdef HAVE_LIBCANBERRA
static ca_context *canberra_context = NULL;
static ca_proplist *canberra_props = NULL;
#endif

static IndicateIndicator *get_indicator (gchar *name);
static IndicateIndicator *add_indicator (gchar *name);

static guint last_new_messages_count = 0;

static void set_indicator_count (IndicateIndicator *indicator, guint count)
{
	if (!indicator)
		return;

	gchar *tmp = g_strdup_printf ("%d", count);
	indicate_indicator_set_property (indicator, INDICATE_INDICATOR_MESSAGES_PROP_COUNT, tmp);
	g_free (tmp);
}

static void update (void)
{
	guint newMessages, unreadMessages, unreadMarkedMessages, totalMessages, markedMessages;
	guint repliedMessages, forwardedMessages, lockedMessages, ignoredMessages, watchedMessages;	

	folder_count_total_msgs (&newMessages, &unreadMessages, &unreadMarkedMessages, &markedMessages, &totalMessages,
				 &repliedMessages, &forwardedMessages, &lockedMessages, &ignoredMessages, &watchedMessages);

	IndicateIndicator *indicator = get_indicator (INDICATOR_NEW_COUNT_NAME);
	if (!indicator)
		return;

	set_indicator_count (indicator, newMessages);
	if (newMessages > 0) {
		indicate_indicator_set_property (indicator, INDICATE_INDICATOR_MESSAGES_PROP_ATTENTION, "true");
		indicate_indicator_show (indicator);
	} else {
		last_new_messages_count = 0;
		indicate_indicator_set_property (indicator, INDICATE_INDICATOR_MESSAGES_PROP_ATTENTION, "false");
		indicate_indicator_hide (indicator);
		return;
	}

	gboolean need_notification = TRUE;
	if (last_new_messages_count >= newMessages)
		/* User is reading/deleting new messages */
		need_notification = FALSE;
	last_new_messages_count = newMessages;

	if (!need_notification)
		return;
#if HAVE_LIBNOTIFY
	if (!notification)
		notification = notify_notification_new (_ ("You've got new mail"), " ", "mail-unread", NULL);
	const gchar *translation = g_dngettext (PACKAGE_NAME, "%d New message received", "%d New messages received", newMessages);
	gchar *title = g_strdup_printf (translation, newMessages);
	notify_notification_update (notification, title, NULL, "notification-message-email");
	notify_notification_set_hint_string (notification, "sound-themed", "message-new-email");
	GError *error = NULL;

	notify_notification_show (notification, &error);
	if (error) {
		debug_print ("Error showing notification: %s\n", error->message);
		g_free (error);
	}
#endif
}

static gboolean folder_item_update_hook (gpointer source, gpointer data)
{
	update ();

	return FALSE;
}

static DbusmenuMenuitem* add_indicator_menu_item (DbusmenuMenuitem *root_menu, gchar *label, GCallback callback, gpointer data)
{
	DbusmenuMenuitem *mi = dbusmenu_menuitem_new ();
	dbusmenu_menuitem_property_set (mi, DBUSMENU_MENUITEM_PROP_LABEL, label);
	if (callback)
		g_signal_connect (G_OBJECT (mi), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, callback, data);
	dbusmenu_menuitem_child_append (root_menu, mi);

	return mi;
}

static void delete_submenu_item (gpointer data, gpointer user_data)
{
	if (!data || !user_data)
		return;

	DbusmenuMenuitem *mi = (DbusmenuMenuitem*)data;
	DbusmenuMenuitem *parent = (DbusmenuMenuitem*)user_data;

	dbusmenu_menuitem_child_delete (parent, mi);
}

static void command_get_mail (DbusmenuMenuitem *mi, guint timestamp, gpointer user_data)
{
	MainWindow *mw = mainwindow_get_mainwindow ();
	inc_all_account_mail (mw, TRUE, TRUE);
}

static void command_new_mail (DbusmenuMenuitem *mi, guint timestamp, gpointer user_data)
{
	MainWindow *mw = mainwindow_get_mainwindow ();
	compose_mail_cb (mw, 0, NULL);
}

static void command_new_mail_account (DbusmenuMenuitem *mi, guint timestamp, gpointer user_data)
{
	compose_new ((PrefsAccount*) user_data, NULL, NULL);
}

static void command_open_addressbook (DbusmenuMenuitem *mi, guint timestamp, gpointer user_data)
{
	addressbook_open (NULL);
}

static void command_exit_claws (DbusmenuMenuitem *mi, guint timestamp, gpointer user_data)
{
	MainWindow *mw = mainwindow_get_mainwindow ();
	if (mw->lock_count == 0)
		app_will_exit (NULL, mw);
}

static void fill_accounts_submenu (DbusmenuMenuitem *parent)
{
	if (!parent)
		return;

	GList *list = dbusmenu_menuitem_get_children (parent);
	if (list)
		g_list_foreach (list, delete_submenu_item, (gpointer)parent);
	list = account_get_list ();

	GList *cur;
	PrefsAccount *acc;
	DbusmenuMenuitem *mi;
	for (cur = list; cur; cur = cur->next) {
		acc = (PrefsAccount*) cur->data;
		add_indicator_menu_item (parent, acc->account_name ? acc->account_name : _ ("Untitled"), G_CALLBACK (command_new_mail_account), acc);
	}
}

static IndicateIndicator *add_indicator (gchar *name)
{
	IndicateIndicator *indicator = indicate_indicator_new ();

	indicate_indicator_set_property (indicator, INDICATE_INDICATOR_MESSAGES_PROP_NAME, name);
	indicate_indicator_show (indicator);

	IndicateIndicator *old = NULL;
	if (indicators)
		old = INDICATE_INDICATOR (g_hash_table_lookup (indicators, name));
	
	if (old) {
		indicate_indicator_hide (old);
		g_free (old);
	}

	indicate_indicator_show (indicator);
	if (!indicators)
		indicators = g_hash_table_new (g_str_hash, g_str_equal);
	
	g_hash_table_insert (indicators, name, indicator);

	return indicator;
}

static IndicateIndicator *get_indicator (gchar *name)
{
	if (!indicators) {
		g_warning ("*INDICATOR*: no indicators?!");
		return NULL;
	}

	return g_hash_table_lookup (indicators, name);
}

gint plugin_init (gchar **error)
{
	if (!check_plugin_version (claws_minimum_version, claws_version_numeric, PACKAGE_NAME, error))
		return -1;

	item_hook_id = hooks_register_hook (FOLDER_ITEM_UPDATE_HOOKLIST, folder_item_update_hook, NULL);
	if (item_hook_id == -1) {
		*error = g_strdup (_ ("Failed to register folder item update hook."));
		return -1;
	}

	indicate_server = indicate_server_ref_default ();
	indicate_server_set_type (indicate_server, "message");
	indicate_server_set_desktop_file (indicate_server, CLAWS_DESKTOP_FILE);
	
	DbusmenuServer *menu_server = dbusmenu_server_new ("/messaging/commands");
	DbusmenuMenuitem *root = dbusmenu_menuitem_new ();

	DbusmenuMenuitem *actions = add_indicator_menu_item (root, _ ("_Actions"), NULL, NULL);
	add_indicator_menu_item (actions, _ ("_Get Mail"), G_CALLBACK (command_get_mail), NULL);
	add_indicator_menu_item (actions, _ ("New _Email"), G_CALLBACK (command_new_mail), NULL);
	fill_accounts_submenu (add_indicator_menu_item (actions, _ ("New E_mail from account"), NULL, NULL));
	add_indicator_menu_item (actions, _ ("Open A_ddressbook"), G_CALLBACK (command_open_addressbook), NULL);
	add_indicator_menu_item (actions, _ ("E_xit Claws Mail"), G_CALLBACK (command_exit_claws), NULL);

	dbusmenu_server_set_root (menu_server, root);
	indicate_server_set_menu (indicate_server, menu_server);

	add_indicator (INDICATOR_NEW_COUNT_NAME);
	update ();
	
#ifdef HAVE_LIBNOTIFY
	if (!notification)
		notify_init (APPLICATION_NAME);
#endif
#ifdef HAVE_LIBCANBERRA
	if (!canberra_context) {
		gint ret = ca_context_create (&canberra_context);
		if (ret < 0)
			debug_print (APPLICATION_NAME ": Canberra context creation error: %s\n", ca_strerror (ret));
		else {
			ret = ca_context_change_props (canberra_context,
						       CA_PROP_APPLICATION_NAME, APPLICATION_NAME,
						       CA_PROP_APPLICATION_ID, APPLICATION_ID,
						       CA_PROP_APPLICATION_VERSION, PACKAGE_VERSION,
						       CA_PROP_WINDOW_X11_XID, ":0",
						       NULL);
			if (ret < 0)
				debug_print (APPLICATION_NAME ": Failed to set canberra properties: %s\n", ca_strerror (ret));
		}
	}
#endif
#ifdef HAVE_UNITY
	unity_launcher = unity_launcher_entry_get_for_desktop_file (CLAWS_DESKTOP_FILE);
#endif
	indicate_server_show (indicate_server);

	debug_print ("Ubuntu indicator plugin loaded\n");
	return 0;
}

gboolean plugin_done (void)
{
	hooks_unregister_hook (FOLDER_ITEM_UPDATE_HOOKLIST, item_hook_id);

	if (indicators) {
		g_hash_table_destroy (indicators);
		indicators = NULL;
	}

	indicate_server_hide (indicate_server);
	g_object_unref (indicate_server);
	indicate_server = NULL;
	
	return TRUE;
}

const gchar *plugin_name (void)
{
	return _ ("Ubuntu Indicator");
}

const gchar *plugin_desc (void)
{
	return _ ("This plugin implements notifications for the latest Ubuntu versions "
		  "which support unity and the integrated indicator applets.");
}

const gchar *plugin_type (void)
{
	return "GTK2";
}

const gchar *plugin_version (void)
{
	return PACKAGE_VERSION;
}

const gchar *plugin_licence (void)
{
	return "GPL2+-compatible";
}

struct PluginFeature *plugin_provides (void)
{
	static struct PluginFeature features[] = {
		{ PLUGIN_NOTIFIER, N_ ("Indicator") },
		{ PLUGIN_NOTHING, NULL }
	};

	return features;
}

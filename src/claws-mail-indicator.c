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
 *
 * ------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
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

/* Workaround for claws-mail including config.h from public headers */
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
#include "claws-mail-indicator-prefs.h"

#define CLAWS_DESKTOP_FILE       "/usr/share/applications/claws-mail.desktop"
#define USER_BLACKLIST_DIR       "indicators/messages/applications-blacklist"
#define USER_BLACKLIST_FILENAME "claws-mail"
#define APPLICATION_NAME        "claws-mail-indicator"
#define APPLICATION_ID          "org.grendel." APPLICATION_NAME

#define INDICATOR_NEW_COUNT_NAME _ ("New messages")
#define INDICATOR_UNREAD_COUNT_NAME _ ("Unread messages")

static guint item_hook_id;
static guint accounts_hook_id;
static IndicateServer *indicate_server = NULL;
static GHashTable *indicators = NULL;
static DbusmenuMenuitem *accounts_menu = NULL;

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
static void remove_indicator (gchar *name);
static IndicateIndicator *add_indicator (gchar *name);
static void fill_accounts_submenu (DbusmenuMenuitem *parent);

static guint last_new_messages_count = 0;

static void set_indicator_count (gchar *name, guint count, gboolean visible, gboolean raise_attention)
{
	IndicateIndicator *indicator = get_indicator (name);
	if (visible) {
		if (!indicator)
			indicator = add_indicator (name);
	} else {
		if (indicator)
			remove_indicator (name);
		if (indicate_indicator_is_visible (indicator))
			indicate_indicator_hide (indicator);
		return;
	}
	
	gchar *tmp = g_strdup_printf ("%d", count);
	indicate_indicator_set_property (indicator, INDICATE_INDICATOR_MESSAGES_PROP_COUNT, tmp);
	g_free (tmp);

	if (count > 0) {
		if (raise_attention)
			indicate_indicator_set_property (indicator, INDICATE_INDICATOR_MESSAGES_PROP_ATTENTION, "true");
		indicate_indicator_show (indicator);
	} else {
		indicate_indicator_set_property (indicator, INDICATE_INDICATOR_MESSAGES_PROP_ATTENTION, "false");
		if (!indicator_prefs.always_show_indicators)
			indicate_indicator_hide (indicator);
	}
}

void update_notifications (void)
{
	guint newMessages, unreadMessages, unreadMarkedMessages, totalMessages, markedMessages;
	guint repliedMessages, forwardedMessages, lockedMessages, ignoredMessages, watchedMessages;	

	folder_count_total_msgs (&newMessages, &unreadMessages, &unreadMarkedMessages, &markedMessages, &totalMessages,
				 &repliedMessages, &forwardedMessages, &lockedMessages, &ignoredMessages, &watchedMessages);

	set_indicator_count (INDICATOR_NEW_COUNT_NAME, newMessages, indicator_prefs.indicator_new_messages, TRUE);
	set_indicator_count (INDICATOR_UNREAD_COUNT_NAME, unreadMessages, indicator_prefs.indicator_unread_messages, FALSE);
	
	gboolean need_notification = TRUE;
	if (newMessages == 0 || last_new_messages_count >= newMessages)
		/* User is reading/deleting new messages */
		need_notification = FALSE;
	last_new_messages_count = newMessages;

	if (!need_notification)
		return;
#if HAVE_LIBNOTIFY
	if (indicator_prefs.notification_bubble) {
		if (!notification)
			notification = notify_notification_new (_ ("You've got new mail"), " ", "mail-unread", NULL);
		const gchar *translation = g_dngettext (PACKAGE_NAME, "%d New message", "%d New messages", newMessages);
		gchar *title = g_strdup_printf (translation, newMessages);
		notify_notification_update (notification, title, NULL, "notification-message-email");
		if (indicator_prefs.notification_sound)
			notify_notification_set_hint_string (notification, "sound-themed", "message-new-email");
		GError *error = NULL;

		notify_notification_show (notification, &error);
		if (error) {
			debug_print ("Error showing notification: %s\n", error->message);
			g_free (error);
		}
	}
#endif
}

static gboolean folder_item_update_hook (gpointer source, gpointer data)
{
	update_notifications ();

	return FALSE;
}

static gboolean update_accounts_list_hook (gpointer source, gpointer data)
{
	GList *child;

	for (child = dbusmenu_menuitem_get_children (accounts_menu); child; child = child->next) {
		dbusmenu_menuitem_child_delete (accounts_menu, child->data);
		g_object_unref (child->data);
	}
	
	fill_accounts_submenu (accounts_menu);
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

static void remove_indicator (gchar *name)
{
	if (!indicators)
		return;

	IndicateIndicator *indicator = g_hash_table_lookup (indicators, name);
	if (!indicator)
		return;

	g_hash_table_remove (indicators, (gconstpointer)name);
	g_object_unref (indicator);
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

	accounts_hook_id = hooks_register_hook (ACCOUNT_LIST_CHANGED_HOOKLIST, update_accounts_list_hook, NULL);
	if (accounts_hook_id == -1) {
		*error = g_strdup (_ ("Failed to register account list change hook."));
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

	accounts_menu = add_indicator_menu_item (actions, _ ("New E_mail from account"), NULL, NULL);
	fill_accounts_submenu (accounts_menu);
	add_indicator_menu_item (actions, _ ("Open A_ddressbook"), G_CALLBACK (command_open_addressbook), NULL);
	add_indicator_menu_item (actions, _ ("E_xit Claws Mail"), G_CALLBACK (command_exit_claws), NULL);

	dbusmenu_server_set_root (menu_server, root);
	indicate_server_set_menu (indicate_server, menu_server);
	
	update_notifications ();
	
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

	claws_mail_indicator_prefs_init ();
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

	claws_mail_indicator_prefs_done ();
	
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

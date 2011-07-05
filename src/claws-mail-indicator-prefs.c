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

#include <defs.h>
#include <prefs.h>
#include <prefs_gtk.h>
#include <prefswindow.h>
#include <gtkutils.h>

/* Workaround for claws-mail including config.h from public headers */
#undef GETTEXT_PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#ifdef HAVE_CONFIG_H
#include "indicator_config.h"
#endif

#include <stdio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "claws-mail-indicator-prefs.h"

typedef struct
{
	PrefsPage page;
	GtkWidget *notification_sound;
	GtkWidget *notification_bubble;
	GtkWidget *indicator_new_messages;
	GtkWidget *indicator_unread_messages;
	GtkWidget *always_show_indicators;
} ClawsMailIndicatorPage;

ClawsMailIndicatorPrefs indicator_prefs;

static PrefParam param[] = {
	{"notification_sound", "TRUE", &indicator_prefs.notification_sound, P_BOOL, NULL, NULL, NULL},
	{"notification_bubble", "FALSE", &indicator_prefs.notification_bubble, P_BOOL, NULL, NULL, NULL},
	{"indicator_new_messages", "TRUE", &indicator_prefs.indicator_new_messages, P_BOOL, NULL, NULL, NULL},
	{"indicator_unread_messages", "FALSE", &indicator_prefs.indicator_unread_messages, P_BOOL, NULL, NULL, NULL},
	{"always_show_indicators", "FALSE", &indicator_prefs.always_show_indicators, P_BOOL, NULL, NULL, NULL},
	{NULL, NULL, NULL, 0, NULL, NULL, NULL}
};

static ClawsMailIndicatorPage prefs_page;

static void create_indicator_prefs_page (PrefsPage *page, GtkWindow *window, gpointer data);
static void destroy_indicator_prefs_page (PrefsPage *page);
static void save_indicator_prefs (PrefsPage *page);

extern void update_notifications (void);

void claws_mail_indicator_prefs_init (void)
{
	static gchar *path[3];
	path[0] = _ ("Plugins");
	path[1] = _ ("Ubuntu Indicator");
	path[2] = NULL;

	prefs_set_default (param);
	gchar *rcpath = g_strconcat (get_rc_dir (), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	prefs_read_config (param, "UbuntuIndicator", rcpath, NULL);
	g_free (rcpath);

	prefs_page.page.path = path;
	prefs_page.page.create_widget = create_indicator_prefs_page;
	prefs_page.page.destroy_widget = destroy_indicator_prefs_page;
	prefs_page.page.save_page = save_indicator_prefs;

	prefs_gtk_register_page ((PrefsPage*)&prefs_page);
}

void claws_mail_indicator_prefs_done (void)
{}

static void create_indicator_prefs_page (PrefsPage *page, GtkWindow *window, gpointer data)
{
	ClawsMailIndicatorPage *prefs_page = (ClawsMailIndicatorPage*)page;

	CLAWS_TIP_DECL ();

	GtkWidget *vbox = gtk_vbox_new (FALSE, VSPACING);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), VBOX_BORDER);
	gtk_widget_show (vbox);

	GtkWidget *tmp;
	GtkWidget *vbox2 = gtkut_get_options_frame (vbox, &tmp, _ ("Notifications"));
	GtkWidget *notification_sound;
	GtkWidget *notification_bubble;

	PACK_CHECK_BUTTON (vbox2, notification_sound, _ ("Play sound"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (notification_sound), indicator_prefs.notification_sound);
	PACK_CHECK_BUTTON (vbox2, notification_bubble, _ ("Show bubble"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (notification_bubble), indicator_prefs.notification_bubble);

	vbox2 = gtkut_get_options_frame (vbox, &tmp, _ ("Indicators"));
	GtkWidget *indicator_new_messages;
	GtkWidget *indicator_unread_messages;
	GtkWidget *always_show_indicators;

	PACK_CHECK_BUTTON (vbox2, indicator_new_messages, _ ("New messages"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (indicator_new_messages), indicator_prefs.indicator_new_messages);
	PACK_CHECK_BUTTON (vbox2, indicator_unread_messages, _ ("Unread messages"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (indicator_unread_messages), indicator_prefs.indicator_unread_messages);
	PACK_CHECK_BUTTON (vbox2, always_show_indicators, _ ("Always show indicators"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (always_show_indicators), indicator_prefs.always_show_indicators);
	CLAWS_SET_TIP (always_show_indicators, _ ("Always show the indicators in Messaging menu, regardless of whether their count is 0."));

	prefs_page->notification_bubble = notification_bubble;
	prefs_page->notification_sound = notification_sound;
	prefs_page->indicator_new_messages = indicator_new_messages;
	prefs_page->indicator_unread_messages = indicator_unread_messages;
	prefs_page->always_show_indicators = always_show_indicators;
	prefs_page->page.widget = vbox;
}

static void destroy_indicator_prefs_page (PrefsPage *page)
{}

static void save_indicator_prefs (PrefsPage *page)
{
	ClawsMailIndicatorPage *prefs_page = (ClawsMailIndicatorPage*)page;
	
	indicator_prefs.notification_sound = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_page->notification_sound));
	indicator_prefs.notification_bubble = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_page->notification_bubble));
	indicator_prefs.indicator_new_messages = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_page->indicator_new_messages));
	indicator_prefs.indicator_unread_messages = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_page->indicator_unread_messages));
	indicator_prefs.always_show_indicators = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs_page->always_show_indicators));

	update_notifications ();
	
	gchar *rcpath = g_strconcat (get_rc_dir (), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
	PrefFile *file = prefs_write_open (rcpath);
	g_free (rcpath);
	if (!file || (prefs_set_block_label (file, "UbuntuIndicator") < 0))
		return;

	if (prefs_write_param (param, file->fp) < 0) {
		g_warning ("Failed to write UbuntuIndicator configuration to file");
		prefs_file_close_revert (file);
		return;
	}

	if (fprintf (file->fp, "\n") < 0) {
		FILE_OP_ERROR (rcpath, "fprintf");
		prefs_file_close_revert (file);
	} else
		prefs_file_close (file);
}

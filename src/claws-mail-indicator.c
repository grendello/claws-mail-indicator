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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <canberra.h>
#include <libnotify/notify.h>
#include <libindicate/server.h>
#include <libindicate/indicator.h>
#include <libindicate/indicator-messages.h>
#include <unity.h>

#include <common/claws.h>
#include <plugin.h>

#undef HAVE_CONFIG_H
#include <folder.h>

#include "claws-mail-version.h"

static guint item_hook_id;

static void update (void)
{
	
}

static gboolean folder_item_update_hook (gpointer source, gpointer data)
{
	update ();

	return FALSE;
}

gint plugin_init (gchar **error)
{
	if (!check_plugin_version (MAKE_NUMERIC_VERSION (3,7,0,0), claws_version_numeric, PACKAGE_NAME, error))
		return -1;

	item_hook_id = hooks_register_hook (FOLDER_ITEM_UPDATE_HOOKLIST, folder_item_update_hook, NULL);
	if (item_hook_id == -1) {
		*error = g_strdup (_ ("Failed to register folder item update hook."));
		return -1;
	}
	
	return 0;
}

gboolean plugin_done (void)
{
	hooks_unregister_hook (FOLDER_ITEM_UPDATE_HOOKLIST, item_hook_id);
	
	return TRUE;
}

const gchar *plugin_name (void)
{
	return _ ("UbuntuIndicator");
}

const gchar *plugin_desc (void)
{
	return _ ("This plugin implements notifications for the latest Ubuntu versions "
		  "which support unity and the integrated indicator applets.");
}

const gchar *plugin_type (void)
{
	return "GTK";
}

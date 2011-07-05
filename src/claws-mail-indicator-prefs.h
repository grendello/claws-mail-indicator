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
#ifndef __CLAWS_MAIL_INDICATOR_PREFS_H
#define __CLAWS_MAIL_INDICATOR_PREFS_H

#include <glib.h>

typedef struct
{
	gboolean notification_sound;
	gboolean notification_bubble;
	gboolean indicator_new_messages;
	gboolean indicator_unread_messages;
	gboolean always_show_indicators;
} ClawsMailIndicatorPrefs;

extern ClawsMailIndicatorPrefs indicator_prefs;

void claws_mail_indicator_prefs_init (void);
void claws_mail_indicator_prefs_done (void);
#endif

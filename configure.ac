#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([claws_mail_indicator_version_major],[0])
m4_define([claws_mail_indicator_version_minor],[2])
m4_define([claws_mail_indicator_version_build],[0])
m4_define([concat], $1$2$3)
m4_define([claws_mail_indicator_version], concat(claws_mail_indicator_version_major.,claws_mail_indicator_version_minor.,claws_mail_indicator_version_build))

AC_PREREQ([2.67])
AC_INIT([claws-mail-indicator], claws_mail_indicator_version, [grendel@twistedcode.net])
AM_INIT_AUTOMAKE([foreign no-define])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_SRCDIR([src/claws-mail-indicator.c])
AM_CONFIG_HEADER([indicator_config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_LIBTOOL

GETTEXT_PACKAGE=claws-mail-indicator
AC_SUBST(GETTEXT_PACKAGE)
AC_SUBST([CONFIG_STATUS_DEPENDENCIES],['$(top_srcdir)/po/LINGUAS'])
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Define to the Gettext package name])
ALL_LINGUAS="`cat "$srcdir/po/LINGUAS" | grep -v '^#'`"

IT_PROG_INTLTOOL([0.34])

AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES([DEPS],[
	indicate-0.5 >= 0.4.91
	claws-mail
	dbus-1
	dbus-glib-1]
)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

PKG_CHECK_MODULES(LIBNOTIFY, 
	[libnotify],
	[AC_DEFINE([HAVE_LIBNOTIFY],[1],[Use libnotify])],
	[AC_MSG_RESULT([no])]
)
AC_SUBST(LIBNOTIFY_CFLAGS)
AC_SUBST(LIBNOTIFY_LIBS)

PKG_CHECK_MODULES(LIBCANBERRA,
	[libcanberra],
	[AC_DEFINE([HAVE_LIBCANBERRA],[1],[Use canberra])],
	[AC_MSG_RESULT([no])]
)
AC_SUBST(LIBCANBERRA_CFLAGS)
AC_SUBST(LIBCANBERRA_LIBS)

PKG_CHECK_MODULES(UNITY,
	[unity],
	[AC_DEFINE([HAVE_UNITY],[1],[Use unity])],
	[AC_MSG_RESULT([no])]
)
AC_SUBST(UNITY_CFLAGS)
AC_SUBST(UNITY_LIBS)

CLAWS_PLUGIN_DIR=`pkg-config --variable=plugindir claws-mail`
AC_SUBST(CLAWS_PLUGIN_DIR)

AC_OUTPUT(
	Makefile
	src/Makefile
	po/Makefile.in
)

if test -n "$LIBNOTIFY_CFLAGS"; then
   LIBNOTIFY_PRESENT=yes
else
   LIBNOTIFY_PRESENT=no
fi

if test -n "$LIBCANBERRA_CFLAGS"; then
   LIBCANBERRA_PRESENT=yes
else
   LIBCANBERRA_PRESENT=no
fi

if test -n "$UNITY_CFLAGS"; then
   UNITY_PRESENT=yes
else
   UNITY_PRESENT=no
fi

cat <<EOF

       Compiler: $CC
 Compiler flags: $CFLAGS
  Use libnotify: $LIBNOTIFY_PRESENT
Use libcanberra: $LIBCANBERRA_PRESENT
      Use unity: $UNITY_PRESENT

EOF

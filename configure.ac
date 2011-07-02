#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([claws-mail-indicator], [0.1.0], [grendel@twistedcode.net])
AM_INIT_AUTOMAKE([foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_SRCDIR(src/claws-mail-indicator.c)
AM_CONFIG_HEADER(config.h)
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

PKG_CHECK_MODULES(DEPS,
                  indicate-0.5 >= 0.4.91
                  claws-mail
                  dbus-1
                  dbus-glib-1
                  libnotify
                  libcanberra                  
                  unity
                  )
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

CLAWS_PLUGIN_DIR=`pkg-config --variable=plugindir claws-mail`
AC_SUBST(CLAWS_PLUGIN_DIR)

AC_OUTPUT(
	Makefile
	src/Makefile
	po/Makefile.in
)


INCLUDES = -I$(top_srcdir) $(DEPS_CFLAGS)

plugindir = $(CLAWS_PLUGIN_DIR)
plugin_LTLIBRARIES = ubuntu-indicator.la

ubuntu_indicator_la_SOURCES = \
	claws-mail-indicator.c \
	claws-mail-version.c \
	claws-mail-version.h

ubuntu_indicator_la_LDFLAGS = -module -avoid-version $(DEPS_LIBS)

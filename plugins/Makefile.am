AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPLUGINDIR='"$(pkglibdir)/plugins"' \
	$(GIGGLE_CFLAGS) $(PLATFORM_CFLAGS) \
	$(WARN_CFLAGS)

AM_LDFLAGS = \
	$(PLATFORM_LDFLAGS)

plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = \
	libpersonal-details.la

noinst_LTLIBRARIES = \
	libhello-world.la

libhello_world_la_SOURCES = \
	giggle-hello-world-plugin.c \
	giggle-hello-world-plugin.h

libpersonal_details_la_SOURCES = \
	giggle-personal-details-window.c \
	giggle-personal-details-window.h



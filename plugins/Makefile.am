AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPLUGINDIR='"$(pkglibdir)/plugins"' \
	$(GIGGLE_CFLAGS) $(PLATFORM_CFLAGS) \
	$(WARN_CFLAGS)

AM_LDFLAGS = \
	$(PLATFORM_LDFLAGS)

plugindir = $(pkglibdir)/plugins
plugin_LTLIBRARIES = libhello-world.la

libhello_world_la_SOURCES = \
	hello-world.c


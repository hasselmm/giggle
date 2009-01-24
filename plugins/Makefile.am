AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DGLADEDIR='"$(pkgdatadir)/glade"' \
	-DPLUGINDIR='"$(pkglibdir)/plugins"' \
	$(GIGGLE_CFLAGS) $(PLATFORM_CFLAGS) \
	$(WARN_CFLAGS)

AM_LDFLAGS = \
	$(PLATFORM_LDFLAGS) \
	-module -rpath "$(pkglibdir)/plugins"

plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = \
	libpersonal-details.la
plugin_DATA = \
	personal-details.xml

noinst_LTLIBRARIES = \
	libhello-world.la

libhello_world_la_SOURCES = \
	giggle-hello-world-plugin.c

libpersonal_details_la_SOURCES = \
	giggle-personal-details-plugin.c \
	giggle-personal-details-window.c \
	giggle-personal-details-window.h

libpersonal_details_la_CFLAGS = $(EBOOK_CFLAGS)
libpersonal_details_la_LIBADD = $(EBOOK_LIBS)

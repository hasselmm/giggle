AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DGLADEDIR='"$(pkgdatadir)/glade"' \
	-DPLUGINDIR='"$(pkglibdir)/plugins"' \
	$(GIGGLE_CFLAGS) $(PLATFORM_CFLAGS) \
	$(WARN_CFLAGS)

AM_LDFLAGS = \
	$(PLATFORM_LDFLAGS) \
	-module -rpath "$(pkglibdir)/plugins" \
	-Wl,--as-needed -Wl,--no-undefined \
	-avoid-version

plugindir = $(pkglibdir)/plugins

plugin_LTLIBRARIES = \
	libpersonal-details.la
dist_plugin_DATA = \
	personal-details.xml

noinst_LTLIBRARIES = \
	libhello-world.la
dist_noinst_DATA = \
	hello-world.xml	    

libhello_world_la_SOURCES = \
	giggle-hello-world-plugin.c
libhello_world_la_LIBADD = \
	../libgiggle/libgiggle.la

libpersonal_details_la_SOURCES = \
	giggle-personal-details-plugin.c \
	giggle-personal-details-window.c \
	giggle-personal-details-window.h

libpersonal_details_la_CFLAGS = $(EBOOK_CFLAGS)
libpersonal_details_la_LIBADD = ../libgiggle-git/libgiggle-git.la $(EBOOK_LIBS)

if ENABLE_VTE

plugin_LTLIBRARIES += \
	libterminal-view.la
dist_plugin_DATA += \
	terminal-view.xml
libterminal_view_la_SOURCES = \
	giggle-view-terminal-plugin.c \
	giggle-view-terminal.c \
	giggle-view-terminal.h \
	$(NULL)

libterminal_view_la_CFLAGS = $(VTE_CFLAGS)
libterminal_view_la_LIBADD = ../libgiggle-git/libgiggle-git.la $(VTE_LIBS)

endif

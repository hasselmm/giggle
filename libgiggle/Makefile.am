libgiggledir = $(pkgincludedir)/libgiggle

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DGLADEDIR='"$(pkgdatadir)/glade"' \
	-DPLUGINDIR='"$(pkglibdir)/plugins/$(VERSION)"' \
	$(GIGGLE_CFLAGS) $(PLATFORM_CFLAGS) \
	$(WARN_CFLAGS)

lib_LTLIBRARIES = libgiggle.la

libgiggle_h_files = \
	giggle-author.h \
	giggle-branch.h \
	giggle-clipboard.h \
	giggle-dispatcher.h \
	giggle-error.h \
	giggle-history.h \
	giggle-job.h \
	giggle-plugin-manager.h \
	giggle-plugin.h \
	giggle-ref.h \
	giggle-remote-branch.h \
	giggle-remote-ref.h \
	giggle-remote.h \
	giggle-revision.h \
	giggle-searchable.h \
	giggle-sysdeps.h \
	giggle-tag.h \
	giggle-view.h \
	giggle-view-shell.h \
	$(NULL)

libgiggle_la_SOURCES = \
	$(BUILT_SOURCES) \
	$(libgiggle_h_files) \
	giggle-author.c \
	giggle-branch.c \
	giggle-clipboard.c \
	giggle-dispatcher.c \
	giggle-error.c \
	giggle-history.c \
	giggle-job.c \
	giggle-plugin-manager.c \
	giggle-plugin.c \
	giggle-ref.c \
	giggle-remote-branch.c \
	giggle-remote-ref.c \
	giggle-remote.c \
	giggle-revision.c \
	giggle-searchable.c \
	giggle-sysdeps.c \
	giggle-tag.c \
	giggle-view.c \
	giggle-view-shell.c \
	$(NULL)

libgiggle_HEADERS = \
	$(libgiggle_git_h_files) \
	giggle-enums.h \
	giggle-marshal.h \
	$(NULL)

libgiggle_la_LIBADD = \
	$(GIGGLE_LIBS)
libgiggle_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-version-info $(LIBGIGGLE_ABI) \
	-Wl,--as-needed -Wl,--no-undefined \
	$(NULL)

giggle-enums.h: giggle-enums.h.in $(libgiggle_h_files)
	( cd $(srcdir) && $(GLIB_MKENUMS) --template $^ ) > xgen-gtbh
	( cmp -s xgen-gtbh $@ || (echo "  ENUM  $@..."; cp xgen-gtbh $@ ))
	( rm -f xgen-gtbh )

giggle-enums.c: giggle-enums.c.in $(libgiggle_h_files)
	( cd $(srcdir) && $(GLIB_MKENUMS) --template $^ ) > xgen-gtbc
	( cmp -s xgen-gtbc $@ || (echo "  ENUM  $@..."; cp xgen-gtbc $@ ))
	( rm -f xgen-gtbc )

giggle-marshal.h: giggle-marshal.list
	( cd $(srcdir) && \
	  $(GLIB_GENMARSHAL) --prefix=giggle_marshal $< --header ) > xgen-gmh
	( cmp -s xgen-gmh $@ || (echo "  GENM  $@..."; cp xgen-gmh $@ ))
	( rm -f xgen-gmh )

giggle-marshal.c: giggle-marshal.list
	( cd $(srcdir) && \
	  echo "#include \"giggle-marshal.h\"" && \
	  $(GLIB_GENMARSHAL) --prefix=giggle_marshal $< --body ) > xgen-gmc
	( cmp -s xgen-gmc $@ || (echo "  GENM  $@..."; cp xgen-gmc $@ ))
	( rm -f xgen-gmc )

BUILT_SOURCES = \
	giggle-marshal.h \
	giggle-marshal.c \
	giggle-enums.h \
	giggle-enums.c

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	giggle-marshal.list \
	giggle-enums.c.in \
	giggle-enums.h.in


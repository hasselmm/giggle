AM_CPPFLAGS =				\
	-I$(top_srcdir)			\
	$(GIGGLE_CFLAGS) 		\
	$(WARN_CFLAGS)

bin_PROGRAMS = giggle

giggle_SOURCES =			\
	giggle-graph-renderer.c giggle-graph-renderer.h	\
	giggle-main.c

giggle_LDADD = 				\
	$(GIGGLE_LIBS)

noinst_PROGRAMS = 			\
	test-patch-view

test_patch_view_SOURCES =		\
	test-patch-view.c

test_patch_view_LDADD = 		\
	$(GIGGLE_LIBS)

giggle-marshal.h: giggle-marshal.list
	(cd $(srcdir) && \
	$(GLIB_GENMARSHAL) --prefix=giggle_marshal giggle-marshal.list --header) > xgen-gmh \
	&& (cmp -s xgen-gmh giggle-marshal.h || cp xgen-gmh giggle-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

giggle-marshal.c: giggle-marshal.list
	(cd $(srcdir) && \
	$(GLIB_GENMARSHAL) --prefix=giggle_marshal giggle-marshal.list --body) > xgen-gmc \
	&& cp xgen-gmc giggle-marshal.c \
	&& rm -f xgen-gmc xgen-gmc~

marshal-main.c: giggle-marshal.c giggle-marshal.h

BUILT_SOURCES =				\
	giggle-marshal.h		\
	giggle-marshal.c

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST =				\
	giggle-marshal.list


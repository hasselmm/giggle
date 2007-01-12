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

marshal.h: giggle-marshal.list
	(cd $(srcdir) && \
	$(GLIB_GENMARSHAL) --prefix=gig_marshal giggle-marshal.list --header) > xgen-gmh \
	&& (cmp -s xgen-gmh marshal.h || cp xgen-gmh giggle-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

marshal.c: giggle-marshal.list
	(cd $(srcdir) && \
	$(GLIB_GENMARSHAL) --prefix=gig_marshal giggle-marshal.list --body) > xgen-gmc \
	&& cp xgen-gmc marshal.c \
	&& rm -f xgen-gmc xgen-gmc~

marshal-main.c: giggle-marshal.c giggle-marshal.h

BUILT_SOURCES =				\
	marshal.h			\
	marshal.c

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST =				\
	marshal.list


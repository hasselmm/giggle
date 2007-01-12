AM_CPPFLAGS =				\
	-I$(top_srcdir)			\
	$(GIGGLE_CFLAGS) 		\
	$(WARN_CFLAGS)

bin_PROGRAMS = giggle

giggle_SOURCES =			\
	gig-main.c

giggle_LDADD = 				\
	$(GIGGLE_LIBS)

gig-marshal.h: gig-marshal.list
	(cd $(srcdir) && \
	$(GLIB_GENMARSHAL) --prefix=gig_marshal gig-marshal.list --header) > xgen-gmh \
	&& (cmp -s xgen-gmh gig-marshal.h || cp xgen-gmh gig-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

gig-marshal.c: gig-marshal.list
	(cd $(srcdir) && \
	$(GLIB_GENMARSHAL) --prefix=gig_marshal gig-marshal.list --body) > xgen-gmc \
	&& cp xgen-gmc gig-marshal.c \
	&& rm -f xgen-gmc xgen-gmc~

gig-marshal-main.c: gig-marshal.c gig-marshal.h

BUILT_SOURCES =				\
	gig-marshal.h			\
	gig-marshal.c

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST =				\
	gig-marshal.list


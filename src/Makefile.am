AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-DGLADEDIR=\""$(datadir)/giggle/glade"\"\
	-DLOCALEDIR=\""$(datadir)/locale"\"	\
	$(GIGGLE_CFLAGS) 			\
	$(WARN_CFLAGS)

bin_PROGRAMS = giggle

giggle_SOURCES =				\
	giggle-dispatcher.c             	\
	giggle-dispatcher.h             	\
	giggle-dummy.c                  	\
	giggle-dummy.h                  	\
	giggle-error.c                          \
	giggle-error.h                          \
	giggle-git.c                            \
	giggle-git.h                            \
	giggle-git-diff.c                       \
	giggle-git-diff.h                       \
	giggle-graph-renderer.c         	\
        giggle-graph-renderer.h	        	\
	giggle-job.c                            \
	giggle-job.h                            \
	giggle-sysdeps.c                        \
	giggle-sysdeps.h                        \
	giggle-revision.c			\
	giggle-revision.h			\
	giggle-window.c				\
	giggle-window.h				\
	giggle-main.c

giggle_LDADD = 					\
	$(GIGGLE_LIBS)

noinst_PROGRAMS = 			        \
	test-graph-renderer                     \
	test-dispatcher                         \
	test-giggle-git

test_graph_renderer_SOURCES =			\
	giggle-revision.c giggle-revision.h	\
	giggle-graph-renderer.c	giggle-graph-renderer.h	\
	test-graph-renderer.c

test_graph_renderer_LDADD =		        \
	$(GIGGLE_LIBS)

test_dispatcher_SOURCES =                       \
	giggle-dispatcher.c                     \
	giggle-dispatcher.h                     \
	giggle-error.c                          \
	giggle-error.h                          \
	giggle-sysdeps.c                        \
	giggle-sysdeps.h                        \
	test-dispatcher.c

test_dispatcher_LDADD =                         \
	$(GIGGLE_LIBS) 

test_giggle_git_SOURCES =                       \
	giggle-dispatcher.c                     \
	giggle-error.c                          \
	giggle-sysdeps.c                        \
	giggle-revision.c                       \
	giggle-git.c                            \
	giggle-git-diff.c                       \
	giggle-job.c                            \
	test-giggle-git.c

test_giggle_git_LDADD =                         \
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

BUILT_SOURCES =					\
	giggle-marshal.h			\
	giggle-marshal.c

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST =					\
	giggle-marshal.list


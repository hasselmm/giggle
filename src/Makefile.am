AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-DGLADEDIR=\""$(datadir)/giggle/glade"\"\
	-DLOCALEDIR=\""$(datadir)/locale"\"	\
	$(GIGGLE_CFLAGS) 			\
	$(WARN_CFLAGS)

bin_PROGRAMS = giggle

dist_ENUMS_TEMPLATES=				\
	giggle-enums.c.in			\
	giggle-enums.h.in

# h_files are the headers that include enums
h_files = 					\
	giggle-remote-branch.h 			\
	giggle-git-refs.h

giggle_SOURCES =				\
	$(BUILT_SOURCES)			\
	eggfindbar.c				\
	eggfindbar.h				\
	giggle-author.c				\
	giggle-author.h				\
	giggle-authors-view.c			\
	giggle-authors-view.h			\
	giggle-branches-view.c			\
	giggle-branches-view.h			\
	giggle-configuration.c			\
	giggle-configuration.h			\
	giggle-description-editor.c		\
	giggle-description-editor.h		\
	giggle-diff-view.c			\
	giggle-diff-view.h			\
	giggle-diff-tree-view.c			\
	giggle-diff-tree-view.h			\
	giggle-dispatcher.c             	\
	giggle-dispatcher.h             	\
	giggle-dummy.c                  	\
	giggle-dummy.h                  	\
	giggle-error.c                          \
	giggle-error.h                          \
	giggle-file-list.c			\
	giggle-file-list.h			\
	giggle-git.c                            \
	giggle-git.h                            \
	giggle-git-authors.c			\
	giggle-git-authors.h			\
	giggle-git-diff.c                       \
	giggle-git-diff.h                       \
	giggle-git-diff-tree.c			\
	giggle-git-diff-tree.h			\
	giggle-git-ignore.c			\
	giggle-git-ignore.h			\
	giggle-git-read-config.c		\
	giggle-git-read-config.h		\
	giggle-git-refs.c			\
	giggle-git-refs.h			\
	giggle-git-revisions.c			\
	giggle-git-revisions.h			\
	giggle-git-write-config.c		\
	giggle-git-write-config.h		\
	giggle-graph-renderer.c         	\
        giggle-graph-renderer.h	        	\
	giggle-job.c                            \
	giggle-job.h                            \
	giggle-personal-details-window.c	\
	giggle-personal-details-window.h	\
	giggle-ref.c				\
	giggle-ref.h				\
	giggle-remote.c				\
	giggle-remote.h				\
	giggle-remote-branch.c			\
	giggle-remote-editor.c			\
	giggle-remote-editor.h			\
	giggle-remotes-view.c			\
	giggle-remotes-view.h			\
	giggle-revision.c			\
	giggle-revision.h			\
	giggle-revision-list.c			\
	giggle-revision-list.h			\
	giggle-revision-tooltip.c		\
	giggle-revision-tooltip.h		\
	giggle-revision-view.c			\
	giggle-revision-view.h			\
	giggle-sysdeps.c                        \
	giggle-sysdeps.h                        \
	giggle-tree-view-helpers.c		\
	giggle-tree-view-helpers.h		\
	giggle-view.c				\
	giggle-view.h				\
	giggle-view-file.c			\
	giggle-view-file.h			\
	giggle-view-history.c			\
	giggle-view-history.h			\
	giggle-view-summary.c			\
	giggle-view-summary.h			\
	giggle-window.c				\
	giggle-window.h				\
	giggle-main.c

giggle_LDADD = 					\
	$(GIGGLE_LIBS)

noinst_PROGRAMS = 			        \
	test-dispatcher                         \
	test-file-list				\
	test-giggle-git

#test_graph_renderer_SOURCES =			\
#	giggle-revision.c giggle-revision.h	\
#	giggle-graph-renderer.c	giggle-graph-renderer.h	\
#	test-graph-renderer.c

#test_graph_renderer_LDADD =		        \
#	$(GIGGLE_LIBS)

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

test_file_list_SOURCES =			\
	$(BUILT_SOURCES)			\
	giggle-dispatcher.c			\
	giggle-error.c				\
	giggle-sysdeps.c			\
	giggle-git.c				\
	giggle-git-diff-tree.c			\
	giggle-git-diff-tree.h			\
	giggle-job.c				\
	giggle-file-list.c			\
	giggle-git-ignore.c			\
	giggle-ref.c				\
	giggle-ref.h				\
	giggle-remote.c				\
	giggle-remote.h				\
	giggle-remote-branch.c			\
	giggle-remote-branch.h			\
	giggle-revision.c			\
	giggle-revision.h			\
	test-file-list.c

test_file_list_LDADD =				\
	$(GIGGLE_LIBS)

test_giggle_git_SOURCES =                       \
	$(BUILT_SOURCES)			\
	giggle-dispatcher.c                     \
	giggle-error.c                          \
	giggle-sysdeps.c                        \
	giggle-ref.c				\
	giggle-revision.c                       \
	giggle-git.c                            \
	giggle-git-diff.c                       \
	giggle-job.c                            \
	giggle-remote.c				\
	giggle-remote.h				\
	giggle-remote-branch.c			\
	giggle-remote-branch.h			\
	test-giggle-git.c

test_giggle_git_LDADD =                         \
	$(GIGGLE_LIBS)

giggle-enums.h: $(h_files) $(priv_h_files) Makefile giggle-enums.h.in
	@( cd $(srcdir) && $(GLIB_MKENUMS) --template giggle-enums.h.in \
	$(h_files) ) >> xgen-gtbh \
	&& (cmp -s xgen-gtbh $@ || (echo "Creating $@..."; cp xgen-gtbh $@) ) \
	&& rm -f xgen-gtbh

giggle-enums.c: $(h_files) $(priv_h_files) Makefile giggle-enums.c.in
	@( cd $(srcdir) && $(GLIB_MKENUMS) --template giggle-enums.c.in \
	$(h_files) ) > xgen-gtbc \
	&& (cmp -s xgen-gtbc $@ || (echo "Creating $@..."; cp xgen-gtbc $@) ) \
	&& rm -f xgen-gtbc


giggle-marshal.h: giggle-marshal.list
	(cd $(srcdir) && \
	$(GLIB_GENMARSHAL) --prefix=giggle_marshal giggle-marshal.list --header) > xgen-gmh \
	&& (cmp -s xgen-gmh giggle-marshal.h || cp xgen-gmh giggle-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

giggle-marshal.c: giggle-marshal.list
	(cd $(srcdir) && \
	echo "#include \"giggle-marshal.h\"" > xgen-gmc \
	&& $(GLIB_GENMARSHAL) --prefix=giggle_marshal giggle-marshal.list --body) >> xgen-gmc \
	&& cp xgen-gmc giggle-marshal.c \
	&& rm -f xgen-gmc xgen-gmc~

marshal-main.c: giggle-marshal.c giggle-marshal.h

BUILT_SOURCES =					\
	giggle-marshal.h			\
	giggle-marshal.c			\
	giggle-enums.h				\
	giggle-enums.c

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST =					\
	giggle-marshal.list			\
	giggle-enums.c.in			\
	giggle-enums.h.in


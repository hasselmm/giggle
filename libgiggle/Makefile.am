AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DGLADEDIR=\""$(datadir)/giggle/glade"\" \
	$(GIGGLE_CFLAGS) \
	$(PLATFORM_CFLAGS) \
	$(WARN_CFLAGS)

lib_LTLIBRARIES = libgiggle.la \
		  libgiggle-git.la

dist_ENUMS_TEMPLATES= \
	giggle-enums.c.in \
	giggle-enums.h.in

# h_files are the headers that include enums
h_files = \
	giggle-remote-branch.h \
	giggle-git-list-files.h

libgiggle_la_SOURCES = \
	$(BUILT_SOURCES) \
	giggle-author.c \
	giggle-author.h \
	giggle-branch.c \
	giggle-branch.h \
	giggle-configuration.c \
	giggle-configuration.h \
	giggle-dispatcher.c \
	giggle-dispatcher.h \
	giggle-dummy.c \
	giggle-dummy.h \
	giggle-error.c \
	giggle-error.h \
	giggle-history.c \
	giggle-history.h \
	giggle-job.c \
	giggle-job.h \
	giggle-ref.c \
	giggle-ref.h \
	giggle-remote.c \
	giggle-remote.h \
	giggle-remote-branch.c \
	giggle-revision.c \
	giggle-revision.h \
	giggle-searchable.c \
	giggle-searchable.h \
	giggle-sysdeps.c \
	giggle-sysdeps.h \
	giggle-tag.c \
	giggle-tag.h \
	giggle-remote-ref.c \
	giggle-remote-ref.h \
	$(NULL)

libgiggle_la_LIBADD= \
	$(GIGGLE_LIBS)
libgiggle_la_LDFLAGS= \
	$(AM_LDFLAGS) \
	-release $(PACKAGE_VERSION) \
	$(NULL)

libgiggle_git_la_SOURCES = \
        giggle-git.c \
	giggle-git.h \
	giggle-git-add.c \
	giggle-git-add.h \
	giggle-git-add-ref.c \
	giggle-git-add-ref.h \
	giggle-git-authors.c \
	giggle-git-authors.h \
	giggle-git-commit.c \
	giggle-git-commit.h \
	giggle-git-delete-ref.c \
	giggle-git-delete-ref.h \
	giggle-git-diff.c \
	giggle-git-diff.h \
	giggle-git-diff-tree.c \
	giggle-git-diff-tree.h \
	giggle-git-ignore.c \
	giggle-git-ignore.h \
	giggle-git-list-files.c \
	giggle-git-list-files.h \
	giggle-git-log.c \
	giggle-git-log.h \
	giggle-git-read-config.c \
	giggle-git-read-config.h \
	giggle-git-refs.c \
	giggle-git-refs.h \
	giggle-git-revisions.c \
	giggle-git-revisions.h \
	giggle-git-write-config.c \
	giggle-git-write-config.h
	 
libgiggle_git_la_LIBADD= \
	$(GIGGLE_LIBS)
libgiggle_git_la_LDFLAGS= \
	$(AM_LDFLAGS) \
	-release $(PACKAGE_VERSION) \
	$(NULL)

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

BUILT_SOURCES = \
	giggle-marshal.h \
	giggle-marshal.c \
	giggle-enums.h \
	giggle-enums.c

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	giggle-marshal.list \
	giggle-enums.c.in \
	giggle-enums.h.in \
	$(h_files)


AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DGLADEDIR=\""$(datadir)/giggle/glade"\" \
	$(GIGGLE_CFLAGS) \
	$(PLATFORM_CFLAGS) \
	$(WARN_CFLAGS)

bin_PROGRAMS = giggle

if IGE_PLATFORM_OSX
GTK_OSX_SOURCES = \
	ige-mac-menu.c \
	ige-mac-menu.h
endif

giggle_SOURCES= \
	$(GTK_OSX_SOURCES) \
	eggfindbar.c \
	eggfindbar.h \
	giggle-authors-view.c \
	giggle-authors-view.h \
	giggle-branches-view.c \
	giggle-branches-view.h \
	giggle-description-editor.c \
	giggle-description-editor.h \
	giggle-diff-view.c \
	giggle-diff-view.h \
	giggle-diff-tree-view.c \
	giggle-diff-tree-view.h \
	giggle-diff-window.c \
	giggle-diff-window.h \
	giggle-file-list.c \
	giggle-file-list.h \
	giggle-graph-renderer.c \
	giggle-graph-renderer.h \
	giggle-input-dialog.c \
	giggle-input-dialog.h \
	giggle-personal-details-window.c \
	giggle-personal-details-window.h \
	giggle-remote-editor.c \
	giggle-remote-editor.h \
	giggle-remotes-view.c \
	giggle-remotes-view.h \
	giggle-rev-list-view.c \
	giggle-rev-list-view.h \
	giggle-revision-tooltip.c \
	giggle-revision-tooltip.h \
	giggle-revision-view.c \
	giggle-revision-view.h \
	giggle-short-list.c \
	giggle-short-list.h \
	giggle-tree-view-helpers.c \
	giggle-tree-view-helpers.h \
	giggle-view.c \
	giggle-view.h \
	giggle-view-file.c \
	giggle-view-file.h \
	giggle-view-history.c \
	giggle-view-history.h \
	giggle-view-summary.c \
	giggle-view-summary.h \
	giggle-window.c \
	giggle-window.h \
	giggle-main.c

LDADD= \
	../libgiggle/libgiggle.la \
	../libgiggle/libgiggle-git.la \
	$(libgiggle_la_LIBADD)

AM_LDFLAGS= \
	$(PLATFORM_LDFLAGS)

#test_graph_renderer_SOURCES = \
#	giggle-revision.c giggle-revision.h \
#	giggle-graph-renderer.c	giggle-graph-renderer.h \
#	test-graph-renderer.c



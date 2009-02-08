icon16dir = $(datadir)/icons/hicolor/16x16/apps

icon16_DATA = \
	16x16/giggle.png \
	16x16/giggle-history-view.png \
	16x16/giggle-scm-git.png \
	16x16/giggle-scm-svn.png \
	16x16/giggle-branch.png \
	16x16/giggle-remote.png \
	16x16/giggle-tag.png

icon22dir = $(datadir)/icons/hicolor/22x22/apps
icon22_DATA = \
	22x22/giggle.png \
	22x22/giggle-history-view.png

icon24dir = $(datadir)/icons/hicolor/24x24/apps
icon24_DATA = \
	24x24/giggle.png \
	24x24/giggle-history-view.png

icon32dir = $(datadir)/icons/hicolor/32x32/apps
icon32_DATA = \
	32x32/giggle.png \
	32x32/giggle-history-view.png

icon48dir = $(datadir)/icons/hicolor/48x48/apps
icon48_DATA = 48x48/giggle.png 48x48/giggle-scm-svn.png

svgicondir = $(datadir)/icons/hicolor/scalable/apps
svgicon_DATA = scalable/giggle.svg

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = \
	$(icon16_DATA) \
	$(icon22_DATA) \
	$(icon24_DATA) \
	$(icon32_DATA) \
	$(icon48_DATA) \
	$(svgicon_DATA)

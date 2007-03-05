gladedir = $(datadir)/giggle/glade
dist_glade_DATA = \
	main-window.glade

desktop_in_data=giggle.desktop.in
desktop_DATA=$(desktop_in_data:.desktop.in=.desktop)
desktopdir=$(datadir)/applications
CLEANFILES=$(desktop_DATA)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST=giggle.desktop.in

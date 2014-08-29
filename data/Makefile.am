# data/Makefile.am for Blackbox - an X11 Window manager

SUBDIRS = styles
pkgdata_DATA = menu

CLEANFILES = menu
EXTRA_DIST = menu.in

menu: menu.in
	$(SED) -e "s,@pkgdatadir@,$(pkgdatadir)," @srcdir@/menu.in > menu


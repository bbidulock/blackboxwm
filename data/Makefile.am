# data/Makefile.am for Blackbox 0.61.x  - an X11 Window manager

SUBDIRS = styles
CLEANFILES = menu
MAINTAINERCLEANFILES = Makefile.in
pkgdata_DATA = menu

all-local: menu
distclean-local:
	rm -f *\~

menu: menu.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," menu.in > menu

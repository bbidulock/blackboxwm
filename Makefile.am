# Makefile.am for Blackbox - an X11 Window manager

AUTOMAKE_OPTIONS = foreign

# disable bsetroot for now - it will be fixed later
# SUBDIRS = data doc nls src util
SUBDIRS = data doc nls src

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in stamp-h.in

uninstall-local:
	-rmdir $(pkgdatadir)

distclean-local:
	rm -f *\~ gmon.out

# Makefile.am for Blackbox - an X11 Window manager

AUTOMAKE_OPTIONS = foreign

SUBDIRS = data doc nls lib src util
MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in stamp-h.in

uninstall-local:
	-rmdir $(pkgdatadir)

distclean-local:
	rm -f *\~ gmon.out .\#*

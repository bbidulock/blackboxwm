# Makefile.am for Blackbox - an X11 Window manager

AUTOMAKE_OPTIONS	= foreign

SUBDIRS			= data \
			  doc \
			  nls \
			  lib \
			  src \
			  util

MAINTAINERCLEANFILES	= Makefile.in \
			  aclocal.m4 \
			  config.h.in \
			  config.guess \
			  config.sub \
			  configure \
			  configure.lineno \
			  depcomp \
			  install-sh \
			  ltmain.sh \
			  missing \
			  mkinstalldirs

uninstall-local:
	-rmdir $(pkgdatadir)

distclean-local:
	rm -f *\~ gmon.out .\#*
	rm -rf autom4te.cache

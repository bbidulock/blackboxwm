# Makefile.am for Blackbox - an X11 Window manager

AUTOMAKE_OPTIONS	= foreign

SUBDIRS			= data \
			  doc \
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

LIBTOOL_DEPS =		@LIBTOOLS_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

uninstall-local:
	-rmdir $(pkgdatadir)

distclean-local:
	rm -f *\~ gmon.out .\#*
	rm -rf autom4te.cache

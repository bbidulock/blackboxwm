# nls/Makefile.am for Blackbox 0.60.x - an X11 Window Manager

NLSTEST = @NLS@
SUBDIRS = C es_ES pt_BR
CLEANFILES = blackbox-nls.hh
MAINTAINERCLEANFILES = Makefile.in

distclean-local:
	rm -f *\~

install-data-hook:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		ln -fs $(pkgdatadir)/nls/C $(pkgdatadir)/nls/US_ASCII; \
		ln -fs $(pkgdatadir)/nls/C $(pkgdatadir)/nls/POSIX; \
		ln -fs $(pkgdatadir)/nls/C $(pkgdatadir)/nls/en_US; \
		ln -fs $(pkgdatadir)/nls/C $(pkgdatadir)/nls/en; \
	fi


# nls/Makefile.am for Blackbox - an X11 Window Manager

NLSTEST = @NLS@
SUBDIRS = C da_DK es_ES et_EE fr_FR pt_BR ru_RU sv_SE tr_TR ja_JP nl_NL it_IT
MAINTAINERCLEANFILES = Makefile.in blackbox-nls.hh
DISTCLEANFILES = blackbox-nls.hh

uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rmdir $(DESTDIR)$(pkgdatadir)/nls; \
	fi
distclean-local:
	rm -f *\~

# nls/Makefile.am for Blackbox - an X11 Window Manager

NLSTEST = @NLS@
SUBDIRS = C da_DK de_DE es_AR es_ES fr_FR hu_HU ja_JP nl_NL \
	  no_NO pl_PL pt_BR ro_RO ru_RU sk_SK sv_SE \
	  zh_CN 
MAINTAINERCLEANFILES = Makefile.in blackbox-nls.hh
DISTCLEANFILES = blackbox-nls.hh

uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rmdir $(DESTDIR)$(pkgdatadir)/nls; \
	fi
distclean-local:
	rm -f *\~ .\#*

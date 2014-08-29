# doc/Makefile.am for Blackbox - an X11 Window manager

SUBDIRS = fr_FR ja_JP nl_NL sl_SI

CLEANFILES = blackbox.1
EXTRA_DIST = blackbox.1.in
man_MANS = blackbox.1
dist_man_MANS = bsetroot.1 bsetbg.1

DEFAULT_MENU=$(pkgdatadir)/menu

distclean-local:
	rm -f *\~ .\#*

blackbox.1: blackbox.1.in
	$(SED) -e "s,@defaultmenu@,$(DEFAULT_MENU)," \
		-e "s,@pkgdatadir@,$(pkgdatadir)," \
		-e "s,@version@,$(VERSION)," \
		 @srcdir@/blackbox.1.in > blackbox.1

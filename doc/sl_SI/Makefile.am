# doc/Makefile.am for Blackbox - an X11 Window manager

CLEANFILES = blackbox.1
MAINTAINERCLEANFILES = Makefile.in
man_MANS = blackbox.1

DEFAULT_MENU=$(pkgdatadir)/menu

distclean-local:
	rm -f *\~

blackbox.1: blackbox.1.in
	@regex_cmd@ -e "s,@defaultmenu@,$(DEFAULT_MENU)," blackbox.1.in > blackbox.1

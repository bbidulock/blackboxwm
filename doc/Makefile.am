# doc/Makefile.am for Blackbox 0.61.x  - an X11 Window manager

CLEANFILES = blackbox.1
MAINTAINERCLEANFILES = Makefile.in
man_MANS = blackbox.1 bsetroot.1

distclean-local:
	rm -f *\~

blackbox.1: blackbox.1.in
	@regex_cmd@ -e "s,@pkgdatadir@,$(pkgdatadir)," blackbox.1.in > blackbox.1

# doc/<locale>/Makefile.am for Blackbox - an X11 Window manager

# what locale is this
locale = nl_NL

# where to install
man1dir = $(mandir)/$(locale)/man1

# what to install
man1_MANS = blackbox.1
dist_man1_MANS = bsetroot.1

CLEANFILES = blackbox.1
EXTRA_DIST = blackbox.1.in

DEFAULT_MENU=$(pkgdatadir)/menu

blackbox.1: blackbox.1.in
	@regex_cmd@ -e "s,@defaultmenu@,$(DEFAULT_MENU)," \
		-e "s,@pkgdatadir@,$(pkgdatadir)," \
		-e "s,@version@,$(VERSION)," \
		$< > $@


# doc/<locale>/Makefile.am for Blackbox - an X11 Window manager

# what locale is this
locale = fr

# where to install
man1dir = $(mandir)/$(locale)/man1

# what to install
if USE_NLS
man1_MANS = blackbox.1
dist_man1_MANS = bsetroot.1
endif

CLEANFILES = blackbox.1
EXTRA_DIST = blackbox.1.in

DEFAULT_MENU=$(pkgdatadir)/menu

blackbox.1: blackbox.1.in
	$(SED) -e "s,@defaultmenu@,$(DEFAULT_MENU)," \
		-e "s,@pkgdatadir@,$(pkgdatadir)," \
		-e "s,@version@,$(VERSION)," \
		$< > $@


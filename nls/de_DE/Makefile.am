# nls/de_DE/Makefile.am for Blackbox - an X11 Window Manager

NLSTEST = @NLS@
CLEANFILES = blackbox.cat
MAINTAINERCLEANFILES = Makefile.in Translation.m
DISTCLEANFILES = Translation.m
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m blackbox.m common.m main.m bsetroot.m

all-local: blackbox.cat
install-data-local: blackbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(DESTDIR)$(pkgdatadir)/nls/de_DE"; \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/nls/de_DE; \
		$(INSTALL_DATA) blackbox.cat $(DESTDIR)$(pkgdatadir)/nls/de_DE; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/de; \
		ln -fs de_DE $(DESTDIR)$(pkgdatadir)/nls/de; \
	fi
uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/de_DE/blackbox.cat; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/de; \
		rmdir $(DESTDIR)$(pkgdatadir)/nls/de_DE; \
	fi

Translation.m: $(MFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		awk -f ../convert.awk output=Translation.m $(MFILES); \
	fi

blackbox.cat: Translation.m
	@if test x$(NLSTEST) = "x-DNLS"; then \
		$(gencat_cmd) blackbox.cat Translation.m; \
	fi

distclean-local:
	rm -f *\~

# nls/nl_NL/Makefile.am for Blackbox - an X11 Window Manager

NLSTEST = @NLS@
CLEANFILES = blackbox.cat
MAINTAINERCLEANFILES = Makefile.in Translation.m
DISTCLEANFILES = Translation.m
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m blackbox.m common.m main.m bsetroot.m

all-local: blackbox.cat
install-data-local: blackbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(DESTDIR)$(pkgdatadir)/nls/nl_NL"; \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/nls/nl_NL; \
		$(INSTALL_DATA) blackbox.cat $(DESTDIR)$(pkgdatadir)/nls/nl_NL; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/nl; \
		ln -fs nl_NL $(DESTDIR)$(pkgdatadir)/nls/nl; \
	fi
uninstall-local:
	@if test x$(NLSTEST) = "x-DNLS"; then \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/nl_NL/blackbox.cat; \
		rm -f $(DESTDIR)$(pkgdatadir)/nls/nl; \
		rmdir $(DESTDIR)$(pkgdatadir)/nls/nl_NL; \
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

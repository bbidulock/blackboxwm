# nls/C/Makefile.am for Blackbox-0.60.x - an X11 Window Manager

NLSTEST = @NLS@
GENCAT_TYPE = @GENCAT_TYPE@
CLEANFILES = blackbox.cat
MAINTAINERCLEANFILES = Makefile.in
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m blackbox.m common.m main.m bsetroot.m

all-local: blackbox.cat
install-data-local: blackbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(pkgdatadir)/nls/C"; \
		$(mkinstalldirs) $(pkgdatadir)/nls/C; \
		$(INSTALL_DATA) blackbox.cat $(pkgdatadir)/nls/C; \
	fi

# note... when adding new catalogs... you do not need the -h ../blackbox-nls.h
# passed to gencat... we only need to generate it once..

blackbox.cat: $(MFILES)
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Generating catalog file using $(GENCAT_TYPE) gencat"; \
		if test "x$(GENCAT_TYPE)" = "xGNU"; then \
			sed -f ../cnv $(MFILES) | \
			gencat --new blackbox.cat - -H \
				../blackbox-nls.hh; \
		else \
			gencat -lang C++ -new blackbox.cat $(MFILES) -h \
				../blackbox-nls.hh; \
		fi \
	fi


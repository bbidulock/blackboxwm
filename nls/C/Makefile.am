# nls/C/Makefile.am for Blackbox-0.60.x - an X11 Window Manager

NLSTEST = @NLS@
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

# note... when adding new catalogs... you do not need the
# header=../blackbox-nls.hh which is passed to awk... we
# only need to generate it once.

Translation.m: $(MFILES)
	awk -f ../convert.awk header=../blackbox-nls.hh \
		output=Translation.m $(MFILES)

blackbox.cat: Translation.m
	gencat blackbox.cat Translation.m

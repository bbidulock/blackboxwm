# nls/sv_SE/Makefile.am for Blackbox-0.60.x - an X11 Window Manager

NLSTEST = @NLS@
GENCAT_TYPE = @GENCAT_TYPE@
CLEANFILES = Translation.m blackbox.cat
MAINTAINERCLEANFILES = Makefile.in
MFILES = BaseDisplay.m Basemenu.m Configmenu.m Icon.m Image.m Screen.m Slit.m Toolbar.m Window.m Windowmenu.m Workspace.m Workspacemenu.m blackbox.m common.m main.m bsetroot.m

all-local: blackbox.cat
install-data-local: blackbox.cat
	@if test x$(NLSTEST) = "x-DNLS"; then \
		echo "Installing catalog in $(pkgdatadir)/nls/sv_SE"; \
		$(mkinstalldirs) $(pkgdatadir)/nls/sv_SE; \
		$(INSTALL_DATA) blackbox.cat $(pkgdatadir)/nls/sv_SE; \
	fi

blackbox.cat: $(MFILES)
				@if test x$(NLSTEST) = "x-DNLS"; then \
	        echo "Generating catalog file using $(GENCAT_TYPE) gencat"; \
	        if test "x$(GENCAT_TYPE)" = "xGNU"; then \
	                sed -f ../gnucnv.sed $(MFILES) | \
	                gencat --new blackbox.cat -; \
	        elif test "x$(GENCAT_TYPE)" = "xSun"; then \
	                awk -f ../suncnv.awk output=Translation.m $(MFILES); \
	                gencat blackbox.cat Translation.m; \
	        else \
	                gencat -lang C++ -new blackbox.cat $(MFILES); \
	        fi \
	fi

# Makefile.am for blackbox-brueghel

SUFFIXES =		.in

bruegheldir =		$(pkgdatadir)/brueghel
MAINTAINERCLEANFILES =	Makefile.in
CLEANFILES =		babel icarus littlebabel rebelangels stylesmenu
brueghel_DATA =		babel babel.gif babel.txt			\
			icarus icarus.gif icarus.txt			\
			littlebabel littlebabel.gif littlebabel.txt	\
			rebelangels rebelangels.gif rebelangels.txt	\
			stylesmenu

distclean-local:
	rm -f */~

all-local: $(brueghel_DATA)

.in:
	@regex_cmd@ -e "s,:pkgdatadir:,$(bruegheldir)," $< > $@

babel: babel.in
icarus: icarus.in
littlebabel: littlebabel.in
rebelangels: rebelangels.in
stylesmenu: stylesmenu.in


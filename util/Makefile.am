# util/Makefile.am for Blackbox - an X11 Window manager

CPPFLAGS= @CPPFLAGS@ @DEBUG@ @NLS@ @TIMEDCACHE@ @ORDEREDPSEUDO@ -I$(top_srcdir)/lib

bin_SCRIPTS = 		bsetbg
bin_PROGRAMS =		bsetroot

bsetroot_SOURCES =	bsetroot.cc
bsetroot_DEPENDENCIES=  $(top_srcdir)/lib/libblackbox.a
bsetroot_LDADD =	-L$(top_srcdir)/lib -lblackbox

MAINTAINERCLEANFILES =	Makefile.in

distclean-local:
	rm -f *\~ .\#*


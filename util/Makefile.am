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

# local dependencies

bsetroot.o: bsetroot.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh \
 $(top_srcdir)/lib/$(top_srcdir)/nls/blackbox-nls.hh $(top_srcdir)/lib/GCCache.hh $(top_srcdir)/lib/BaseDisplay.hh \
 $(top_srcdir)/lib/Timer.hh $(top_srcdir)/lib/Color.hh $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh \
 bsetroot.hh $(top_srcdir)/lib/Image.hh

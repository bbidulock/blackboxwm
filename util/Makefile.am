# util/Makefile.am for Blackbox - an X11 Window manager

CPPFLAGS= @CPPFLAGS@ @DEBUG@ @NLS@ @TIMEDCACHE@ @ORDEREDPSEUDO@ -I../lib

bin_SCRIPTS = 		bsetbg
bin_PROGRAMS =		bsetroot

bsetroot_SOURCES =	bsetroot.cc
bsetroot_DEPENDENCIES=  ../lib/libblackbox.a
bsetroot_LDADD =	-L../lib -lblackbox

MAINTAINERCLEANFILES =	Makefile.in

distclean-local:
	rm -f *\~ .\#*

# local dependencies

bsetroot.o: bsetroot.cc ../config.h ../lib/i18n.hh \
 ../lib/../nls/blackbox-nls.hh ../lib/GCCache.hh ../lib/BaseDisplay.hh \
 ../lib/Timer.hh ../lib/Color.hh ../lib/Texture.hh ../lib/Util.hh \
 bsetroot.hh ../lib/Image.hh

# util/Makefile.am for Blackbox - an X11 Window manager

CPPFLAGS= @CPPFLAGS@ @DEBUG@ @NLS@ @TIMEDCACHE@ @NEWWMSPEC@ @INTERLACE@ \
@ORDEREDPSEUDO@

bin_SCRIPTS = 		bsetbg
bin_PROGRAMS =		bsetroot

bsetroot_SOURCES =	bsetroot.cc
bsetroot_LDADD =	../src/BaseDisplay.o ../src/LinkedList.o ../src/Timer.o ../src/Image.o ../src/i18n.o

MAINTAINERCLEANFILES =	Makefile.in

distclean-local:
	rm -f *\~

bsetroot.o: bsetroot.cc ../config.h bsetroot.hh ../src/BaseDisplay.hh \
 ../src/LinkedList.hh ../src/Timer.hh ../src/Image.hh

# util/Makefile.am for Blackbox 0.60.x  - an X11 Window manager

CPPFLAGS= @CPPFLAGS@ @DEBUG@

bin_PROGRAMS =		bsetroot

bsetroot_SOURCES =	bsetroot.cc
bsetroot_LDADD =	../src/BaseDisplay.o ../src/LinkedList.o ../src/Timer.o ../src/Image.o

MAINTAINERCLEANFILES =	Makefile.in

distclean-local:
	rm -f *\~

bsetroot.o: bsetroot.cc ../config.h bsetroot.hh ../src/BaseDisplay.hh \
 ../src/LinkedList.hh ../src/Timer.hh ../src/Image.hh

#---------------------------------------------------------------------------
# (C) 1997 - 1998 Armin Biere
#     $Id: Makefile,v 1.1 2001/11/30 07:54:36 shaleh Exp $
#---------------------------------------------------------------------------


# callchain.c needs `gcc' at least for non x86 systems

CC=gcc
#CFLAGS=-g -DDEBUG -Wall
#CFLAGS= -O2 -Wall
CFLAGS= -O -g -Wall

CXXFLAGS=$(CFLAGS)
CXX=gcc

.SUFFIXES: .cc .o

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $*.cc

# uncomment assert.o if -DDEBUG is used in CFLAGS

OBJS=callchain.o hash.o wrapper.o assert.o

all: libccmalloc.a ccmalloc.o

CCMALLOCSRCS= \
 ccmalloc.cc hash.c hash.h callchain.c assert.h \
 assert.c wrapper.c CtorDtor.cc

ccmalloc.o: config.h $(CCMALLOCSRCS)
	$(CXX) -c $(CXXFLAGS) -fPIC -DUSE_DTOR_FOR_REPORTING ccmalloc.cc

libccmalloc.a: config.h $(OBJS)
	ar cr $@ $(OBJS)
	ranlib $@

wrapper.o: wrapper.c
	$(CC) $(CFLAGS) -c \
	  -DFREE=callchain_free -DMALLOC=callchain_malloc wrapper.c

config.h: configure
	./configure

clean:
	rm -f $(OBJS) libccmalloc.a ccmalloc.o core a.out tags config.h
	cd ../test; make clean

install: libccmalloc.a ccmalloc.o
	install libccmalloc.a $(PREFIX)
	install ccmalloc.o $(PREFIX)
	install ../ccmalloc.cfg $(PREFIX)/ccmalloc.cfg

test: libccmalloc.a ccmalloc.o
	cd ../test; ./testall

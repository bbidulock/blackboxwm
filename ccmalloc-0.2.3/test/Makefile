CC=gcc
CFLAGS=-g -Wall 
CXX=$(CC)
CXXFLAGS=$(CFLAGS)

.SUFFIXES: .exe .c .cc

.cc.exe:
	$(CXX) $(CXXFLAGS) -o $@ $*.cc ../src/ccmalloc.o -ldl
.c.exe:
	$(CC) $(CFLAGS) -o $@ $*.c -L../src -lccmalloc -ldl
all:
	./testall
clean:
	rm -f *.exe *.log *.log.gz *.log.* .ccmalloc core a.out
	rm -f lib_test_C_11.a lib_test_C_19.so *.o

lib_test_C_11.a: lib_test_C_11.o
	ar cr $@ lib_test_C_11.o
	ranlib $@

test_C_11.exe: lib_test_C_11.a test_C_11.o
	$(CC) $(CFLAGS) -o $@ $*.o \
	  -L. -l_test_C_11 \
	  -L../src -lccmalloc -ldl

lib_test_C_19.so: lib_test_C_19.c
	$(CC) $(CFLAGS) -fPIC -c -o lib_test_C_19.o lib_test_C_19.c
	gcc -shared -WL,-soname,$@ -o $@ lib_test_C_19.o

test_C_19.exe: test_C_19.o lib_test_C_19.so
	$(CC) $(CLAGS) -o $@ test_C_19.o -L. -l_test_C_19 \
	  -L../src/ -lccmalloc -ldl

# no debugging info

test_C_13.exe: test_C_13.c
	$(CC) -o $@ -O2 $*.c -L../src -lccmalloc -ldl

test_C_14.exe: test_C_14.c
	gcc -o $@ -fomit-frame-pointer -O2 $*.c -L../src -lccmalloc -ldl
	

#include <stdio.h>
#include <stdlib.h>

static char * mkstr(char * s) { return strcpy(malloc(strlen(s)+1), s); }

void f() { mkstr("asdfasdf"); }

void g()
{
  int i;
  char * a[100];

  for(i=0; i<100; i++)
    a[i] = mkstr("in f");
  
  free(a[2]);
  free(a[0]);
}

int
main()
{
  char * a = mkstr("Hallo"), *b;

  (void) mkstr("Test");

  f();

  b = mkstr("Test");

  g();

  a[6]=0;

  free(b);
  free(a);

  exit(0);
  return 1;
}

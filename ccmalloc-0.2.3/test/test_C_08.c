#include <stdlib.h>

int
main()
{
  char * a = malloc(100);
  char * b = malloc(100);
  char * c = malloc(100);
  char * d = malloc(100);
  char * e = malloc(100);
  char * f = malloc(100);
  char * g = malloc(100);
  char * h = malloc(100);
  char * i = malloc(100);
  char * j = malloc(100);
  char * k = malloc(100);
  char * l = malloc(100);
  char * m = malloc(100);
  char * n = malloc(100);
  char * o = malloc(100);
  char * p = malloc(100);
  char * q = malloc(100);

  free(q);
  free(p);
  free(o);
  free(n);
  free(m);

  a[120] = 2;			/* BOOM */

  free(l);
  free(k);
  free(j);
  free(i);
  free(h);

  g[-9] = 2;			/* BOOM */

  free(g);
  free(f);
  free(e);
  free(d);
  free(c);
  free(b);
  free(a);

  exit(0);
  return 0;
}

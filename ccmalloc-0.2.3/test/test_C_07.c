#include <stdlib.h>

int
main()
{
  char * a = malloc(10);
  free(a);
  a[2]=123;
  free(malloc(2));

  exit(0);
  return 0;
}

#include <stdio.h>
#include <stdlib.h>

int
main()
{
  char * s = (char*) malloc(15);
  FILE * file = fopen("/dev/null", "w");
  free(s);
  fprintf(file, "hello world\n");
  exit(0);
  return 0;
}

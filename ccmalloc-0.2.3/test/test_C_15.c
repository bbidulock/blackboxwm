#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char * str = "this is a string";

int
main()
{
  char * s = strdup(str);
  s = strdup(s);
  free(s);

  exit(0);
  return 0;
}

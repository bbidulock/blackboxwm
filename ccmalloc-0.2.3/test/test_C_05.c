#include <stdlib.h>
#include <string.h>

#define S "Hello World"

int main()
{
  char * s = strcpy((char*) malloc(strlen(S)), S);
  free(s);
  return 0;
  exit(0);
}

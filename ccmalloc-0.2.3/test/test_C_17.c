/* This is mainly a test for logpid
 */

#include <stdlib.h>
#include <stdio.h>

#define NUM_PRG_TO_START 9

int main(int argc, char ** argv)
{
  int n;
  char buffer[20];
  char * leak, * no_leak;

  if(argc == 1)
    {
      n = NUM_PRG_TO_START;
      printf("  started  0");
    }
  else
    {
      n = atoi(argv[1]);
      printf("%d", NUM_PRG_TO_START - n);
    }

  fflush(stdout);

  leak = (char*) malloc(4711);
  no_leak = (char*) malloc(42);

  if(n > 0)
    {
      sprintf(buffer, "%s %d", argv[0], n - 1);
      system(buffer);
    }
  else printf("\n  finished  ");

  free(no_leak);

  printf("%d", n);
  if(n == NUM_PRG_TO_START) printf("\n");
  fflush(stdout);

  exit(0);
  return 1;
}

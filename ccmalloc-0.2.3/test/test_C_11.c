#include <stdlib.h>

extern void * MALLOC(size_t n);

int
main()
{
  void * v = MALLOC(15);
  free(v);

  v=malloc(18);
  free(v);

  exit(0);
  return 0;
}

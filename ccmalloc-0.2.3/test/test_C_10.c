extern void ccmalloc_check_for_integrity();

#include <stdlib.h>

int
main()
{
  char * a = malloc(100);
  a[-5]='0';			/* this is exactly in the second
  				 * word below `a' on 32 bit machines
				 */
  ccmalloc_check_for_integrity();

  malloc(20);
  malloc(30);
  malloc(40);
  malloc(50);

  exit(0);
  return 0;
}

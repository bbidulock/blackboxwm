/* Under SunOS and Solaris a double must be 8 Byte aligned. This means
 * we have to align all data to 8 Bytes :-(
 */

#include <stdlib.h>

struct Test { double d; };

int main()
{
  struct Test * test = (struct Test*) malloc(sizeof(struct Test));
  test -> d = 0.0;
  free(test);

  exit(0);
  return 1;
}

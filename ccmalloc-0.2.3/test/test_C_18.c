#include <stdlib.h>

int num()
{
  int res;

  res = rand();
  if(res < 0) res = -res;
  res %= 10;

  return res;
}

void i()
{
  (void) malloc(10);
}

void j()
{
  (void) malloc(11);
}

void h()
{
  if(rand() % 2 == 0) i();
  else j();
}

void g()
{
  switch(num())
    {
      case 0: h(); break;
      case 1: i(); break;
      case 2: j(); break;
      case 3: h(); break;
      case 4: i(); break;
      case 5: j(); break;
      case 6: h(); break;
      case 7: i(); break;
      case 8: j(); break;
      default: h(); break;
    }
}

void f()
{
  switch(num())
    {
      case 0: g(); break;
      case 1: h(); break;
      case 2: i(); break;
      case 3: j(); break;
      case 4: g(); break;
      case 5: h(); break;
      case 6: i(); break;
      case 7: j(); break;
      case 8: g(); break;
      default: h(); break;
    }
}

int main()
{
  int i;

  for(i=0; i<1000; i++) f();

  exit(0);
  return 0;
}

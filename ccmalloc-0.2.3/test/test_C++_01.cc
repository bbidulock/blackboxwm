extern "C" {
#include <string.h>
#include <stdlib.h>
};

class Wrong
{
protected:

  char * str;

public:

  Wrong(const char * s)
  {
    strcpy(str = new char [ strlen(s) + 1 ], s);
  }
};

class Right
:
  public Wrong
{
public:
  
  Right(const char * s) : Wrong(s) { }
  ~Right() { delete [] str; }
};

Wrong wrong("first test");
Right right("right");

int
main()
{
  Wrong a("a second test with a longer string");
  Wrong b("short string");
  (void) a; (void) b;

  return 0;
  exit(0);
}

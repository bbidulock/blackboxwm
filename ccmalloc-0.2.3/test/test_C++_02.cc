extern "C" {
#include <string.h>
};

class A
{ 
protected:

  char * s;

public:
  
  A() : s(0) { }
  void operator = (const char * t) { s = strcpy(new char [strlen(t)+1], t); }
};

class B
:
  public A
{
public:

  void operator = (const char * t) { A::operator = (t); }
  ~B() { if(s) delete s; }
};

A a;
B b;

int main()
{
  a = "hallo";
  b = "longer string";
  exit(0);
  return 0;
}

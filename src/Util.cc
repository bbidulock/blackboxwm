#include "Util.hh"

#include <string.h> // C string
#include <string>   // C++ string
using std::string;

char* expandTilde(const char* s) {
  if (s[0] != '~') return bstrdup(s);

  string ret = getenv("HOME");
  char* path = strchr(s, '/');
  ret += path;
  return bstrdup(ret.c_str()); 
}

char* bstrdup(const char *s) {
  const size_t len = strlen(s) + 1;
  char *n = new char[len];
  strcpy(n, s);
  return n;
}

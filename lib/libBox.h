

#ifdef _libBox_h
#define _libBox_h

#include <X11/Xlib.h>

#include "libBoxdefs.h"


typedef struct BlackboxInterface {
  Atom _BLACKBOX_MESSAGE;
  Display *display;
} BlackboxInterface;


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif // _libBox_h


#ifndef __libBox_h
#define __libBox_h

#include "libBoxdefs.h"

#include <X11/Xlib.h>


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
  
  Atom _BLACKBOX_MESSAGE, _BLACKBOX_CONTROL;
  
  Bool BlackboxAddWindow(Display *, Window);
  Bool BlackboxConfirmControl(Display *, Window);
  Bool BlackboxOpenApp(Display *, Window);
  Bool BlackboxRemoveWindow(Display *, Window);
  
#if defined(__cplusplus) || defined(c_plusplus)
}
#endif


#endif // __libBox_h


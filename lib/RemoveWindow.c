
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#include "libBox.h"

#include <stdio.h>


static Bool removeWindow(Display *d, XEvent *e, XPointer arg) {
  if (e->type == ClientMessage)
    if (e->xclient.message_type == _BLACKBOX_MESSAGE &&
        e->xclient.format == 32)
      if (e->xclient.data.l[0] == __blackbox_removeTopLevelWindow &&
          (unsigned) e->xclient.data.l[1] == *((Window *) arg) &&
          e->xclient.data.l[2] == __blackbox_accept)
        return True;

  return False;
}


Bool BlackboxRemoveWindow(Display *display, Window window) {
  if (display && window) {
    Atom atom_return;
    Window controlWindow;
    int format_return;
    unsigned long ulfoo;
    unsigned char *data;

    // check for the _BLACKBOX_CONTROL property on the root window, this lets
    // us know that Blackbox is running...

    XGetWindowProperty(display, DefaultRootWindow(display), _BLACKBOX_CONTROL,
		       0, 2, False, _BLACKBOX_CONTROL, &atom_return, 
		       &format_return, &ulfoo, &ulfoo, &data);
    
    if (data) {
      controlWindow = *((Window *) data);
      XFree(data);

      // tell blackbox we want to be removed
      if (format_return == 32 && atom_return == _BLACKBOX_CONTROL) {
	XEvent e;
	
	e.type = ClientMessage;
	e.xclient.display = display;
	e.xclient.window = window;
	e.xclient.message_type = _BLACKBOX_MESSAGE;
	e.xclient.format = 32;
	e.xclient.data.l[0] = __blackbox_removeTopLevelWindow;
	e.xclient.data.l[1] = window;
	e.xclient.data.l[2] = 0l;
	e.xclient.data.l[3] = 0l;
	e.xclient.data.l[4] = 0l;
	
	XSendEvent(display, controlWindow, False, NoEventMask, &e);
	XIfEvent(display, &e, removeWindow, (XPointer) &window);
	return True;
      }
    }
  } else
    puts("RemoveWindow.c: BlackboxRemoveWindow(): invalid display/window "
	 "arguments");

  return False;
}

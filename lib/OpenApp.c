
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#include "libBox.h"

#include <stdio.h>


static Bool confirmControl(Display *d, XEvent *e, XPointer arg) {
  if (e->type == ClientMessage)
    if (e->xclient.message_type == _BLACKBOX_MESSAGE &&
        e->xclient.format == 32)
      if (e->xclient.data.l[0] == __blackbox_confirmControl &&
          (unsigned) e->xclient.data.l[1] == *((Window *) arg) &&
          e->xclient.data.l[2] == __blackbox_accept)
        return True;

  return False;
}


Bool BlackboxConfirmControl(Display *display, Window window) {
  if (display && window) {
    Atom atom_return;
    Window controlWindow;
    int format_return;
    unsigned long ulfoo;
    unsigned char *data;

    // check for the _BLACKBOX_CONTROL property on the root window, this lets
    // us know that Blackbox is running...

    _BLACKBOX_CONTROL = XInternAtom(display, "BLACKBOX_CONTROL", False);
    _BLACKBOX_MESSAGE = XInternAtom(display, "BLACKBOX_MESSAGE", False);

    XGetWindowProperty(display, DefaultRootWindow(display), _BLACKBOX_CONTROL,
		       0, 2, False, _BLACKBOX_CONTROL, &atom_return, 
		       &format_return, &ulfoo, &ulfoo, &data);

    if (data) {
      controlWindow = *((Window *) data);
      XFree(data);

      if (format_return == 32 && atom_return == _BLACKBOX_CONTROL) {
        // the property is set... lets tell blackbox that we're friendly :)
        XEvent e;

        e.type = ClientMessage;
        e.xclient.display = display;
        e.xclient.window = window;
        e.xclient.message_type = _BLACKBOX_MESSAGE;
        e.xclient.format = 32;
        e.xclient.data.l[0] = __blackbox_confirmControl;
        e.xclient.data.l[1] = window;
        e.xclient.data.l[2] = 0l;
        e.xclient.data.l[3] = 0l;
        e.xclient.data.l[4] = 0l;

        XSendEvent(display, controlWindow, True, NoEventMask, &e);
	
        // Bool predicate() checks to make sure that blackbox sends back the
        // same event we just send... except with BlackboxAccept in data.l[2]
        XIfEvent(display, &e, confirmControl, (XPointer) &window);
	return True;
      }
    }
  } else
    puts("OpenApp.c: BlackboxOpen(): invalid display/window arguments");

  return False;
}


Bool BlackboxOpenApp(Display *display, Window window) {
  if (BlackboxConfirmControl(display, window))
    if (BlackboxAddWindow(display, window))
      return True;
  
  return False;
}

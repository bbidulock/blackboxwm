//
// ReconfigureWidget.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// (See the included file COPYING / GPL-2.0)
//

#define _GNU_SOURCE
#include <X11/Xlib.h>

#include "ReconfigureWidget.hh"
#include "blackbox.hh"


ReconfigureWidget::ReconfigureWidget(Blackbox *bb) {
  blackbox = bb;
  display = blackbox->control();

  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect |CWCursor|CWEventMask; 
  
  attrib_create.background_pixmap = None;
  attrib_create.background_pixel = blackbox->toolboxColor().pixel;
  attrib_create.border_pixel = blackbox->toolboxTextColor().pixel;
  attrib_create.override_redirect = False;
  attrib_create.cursor = blackbox->sessionCursor();
  attrib_create.event_mask = SubstructureRedirectMask|ButtonPressMask|
    ButtonReleaseMask|ButtonMotionMask|ExposureMask|EnterWindowMask;
  
  DialogText[0] =
    "Blackbox has the ability to perform an automatic reconfiguration, but";
  DialogText[1] =
    "this capability has proven unreliable in the past.  Choose \"Yes\" to";
  DialogText[2] =
    "allow Blackbox to reconfigure itself.  WARNING:  This can cause Blackbox";
  DialogText[3] =
    "to dump core and abort.  NOTE:  Previous bugs have been attributed to";
  DialogText[4] =
    "improper event handling when reconfiguring automatically.  Choose \"No\"";
  DialogText[5] =
    "and either restart or choose the Reconfigure option from your root menu.";

  line_h = blackbox->titleFont()->ascent + blackbox->titleFont()->descent + 6;
  text_w = 0;
  text_h = line_h * 6;
 
  for (int i = 0; i < 6; i++) {
    int tmp = XTextWidth(blackbox->titleFont(), DialogText[i],
			 strlen(DialogText[i])) + 6;
    text_w = ((text_w < tmp) ? tmp : text_w);
  }

  XSizeHints *rsizehint = XAllocSizeHints();
  rsizehint->flags = PPosition|PSize|PMinSize|PMaxSize;
  rsizehint->x = rsizehint->y = 200;
  rsizehint->width = rsizehint->min_width = rsizehint->max_width = text_w + 10;
  rsizehint->height = rsizehint->min_height = rsizehint->max_height =
    text_h + (line_h * 3);

  window = XCreateWindow(display, blackbox->Root(), 0, 0, text_w + 10,
			 text_h + (line_h * 3), 0, blackbox->Depth(),
			 InputOutput, blackbox->visual(), create_mask,
			 &attrib_create);
  XStoreName(display, window, "Perform Auto-Reconfiguration?");
  XSetWMNormalHints(display, window, rsizehint);
  XFree(rsizehint);

  text_window = XCreateWindow(display, window, 5, 5, text_w, text_h, 0,
			      blackbox->Depth(), InputOutput,
			      blackbox->visual(), create_mask, &attrib_create);

  yes_button =  XCreateWindow(display, window, 5, text_h + line_h,
			      (text_w / 2) - 10, line_h, 1, blackbox->Depth(),
			      InputOutput, blackbox->visual(), create_mask,
			      &attrib_create);

  no_button =  XCreateWindow(display, window, (text_w / 2) + 10,
			     text_h + line_h, (text_w / 2) - 10, line_h, 1,
			     blackbox->Depth(), InputOutput,
			     blackbox->visual(), create_mask, &attrib_create);

  XGCValues gcv;
  gcv.font = blackbox->titleFont()->fid;
  gcv.foreground = blackbox->toolboxTextColor().pixel;
  dialogGC = XCreateGC(display, text_window, GCFont|GCForeground, &gcv);

  blackbox->saveRWidgetSearch(window, this);
  blackbox->saveRWidgetSearch(text_window, this);
  blackbox->saveRWidgetSearch(yes_button, this);
  blackbox->saveRWidgetSearch(no_button, this);

  assocWidget = new BlackboxWindow(blackbox, window, True);
  XMapSubwindows(display, window);
}


ReconfigureWidget::~ReconfigureWidget(void) {
  blackbox->removeRWidgetSearch(window);
  blackbox->removeRWidgetSearch(text_window);
  blackbox->removeRWidgetSearch(yes_button);
  blackbox->removeRWidgetSearch(no_button);

  XFreeGC(display, dialogGC);
  XDestroyWindow(display, yes_button);
  XDestroyWindow(display, no_button);
  XDestroyWindow(display, text_window);
  XDestroyWindow(display, window);

  blackbox->removeWindow(assocWidget);
  delete assocWidget;
}


void ReconfigureWidget::Show(void) {
  XMoveWindow(display, assocWidget->frameWindow(),
	      (blackbox->XResolution() / 2) - ((text_w + 10) / 2),
	      (blackbox->YResolution() / 2) - ((text_h + (line_h * 3)) / 2));
  XMapSubwindows(display, window);
  XMapWindow(display, window);

  blackbox->raiseWindow(assocWidget);
  assocWidget->deiconifyWindow();
}


void ReconfigureWidget::buttonPressEvent(XButtonEvent *be) {
  if (be->window == yes_button) {
    blackbox->Reconfigure();
    delete this;
  } else if (be->window == no_button) {
    delete this;
  }
}


void ReconfigureWidget::buttonReleaseEvent(XButtonEvent *) {

}


void ReconfigureWidget::exposeEvent(XExposeEvent *ee) {
  if (ee->window == text_window)
    for (int i = 0; i < 6; i++)
      XDrawString(display, text_window, dialogGC, 3, (line_h - 3) * (i + 1),
		  DialogText[i], strlen(DialogText[i]));
  else if (ee->window == yes_button)
    XDrawString(display, yes_button, dialogGC, 3, line_h - 3, "Yes", 3);
  else if (ee->window == no_button)
    XDrawString(display, no_button, dialogGC, 3, line_h - 3, "No", 2);
}

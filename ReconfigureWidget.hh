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

#ifndef __ReconfigureWidget_hh
#define __ReconfigureWidget_hh

#include <X11/Xlib.h>

// forward declaration
class ReconfigureWidget;

class Blackbox;
class BlackboxWindow;


class ReconfigureWidget {
private:
  Blackbox *blackbox;
  BlackboxWindow *assocWidget;

  Bool visible;
  Display *display;
  GC dialogGC;
  Window window, text_window, yes_button, no_button;
  char *DialogText[6];
  int text_w, text_h, line_h;
  

protected:


public:
  ReconfigureWidget(Blackbox *);
  ~ReconfigureWidget(void);

  Bool Visible(void) { return visible; }

  void Show(void);
  void Hide(void);

  void buttonPressEvent(XButtonEvent *);
  void buttonReleaseEvent(XButtonEvent *);
  void exposeEvent(XExposeEvent *);
};


#endif // __ReconfigureWidget_hh

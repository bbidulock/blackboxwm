//
// Application.cc for Blackbox - an X11 Window manager
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#include <X11/Xlib.h>

#include "blackbox.hh"
#include "Application.hh"
#include "WorkspaceManager.hh"

#include "../lib/libBoxdefs.h"


Application::Application(Blackbox *bb, Window w) {
  blackbox = bb;
  display = blackbox->control();
  topLevelWindow = w; 

  blackbox->saveAppSearch(topLevelWindow, this);
  blackbox->workspaceManager()->addApplication(this);
}


Application::~Application(void) {
  blackbox->removeAppSearch(topLevelWindow);
  blackbox->workspaceManager()->removeApplication(this);
}


void Application::clientMessageEvent(XClientMessageEvent *xclient) {
  if (xclient->data.l[0] == __blackbox_removeTopLevelWindow &&
      (unsigned) xclient->data.l[1] == topLevelWindow) {
    XEvent e;
    e.type = ClientMessage;
    e.xclient.display = display;
    e.xclient.window = topLevelWindow;
    e.xclient.message_type = xclient->message_type;
    e.xclient.format = 32;
    e.xclient.data.l[0] = __blackbox_removeTopLevelWindow;
    e.xclient.data.l[1] = topLevelWindow;
    e.xclient.data.l[2] = __blackbox_accept;

    XSendEvent(display, topLevelWindow, False, NoEventMask, &e);
    delete this;
  }
}


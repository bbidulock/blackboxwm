//
// Slit.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999, 1999 by Brad Hughes, bhughes@tcac.net
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

#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#ifdef SLIT

#include "blackbox.hh"
#include "Image.hh"
#include "Screen.hh"
#include "Slit.hh"
#include "Toolbar.hh"


Slit::Slit(Blackbox *bb, BScreen *scr) {
  blackbox = bb;
  screen = scr;

  display = screen->getDisplay();
  frame.window = frame.pixmap = None;

  clientList = new LinkedList<SlitClient>;

  slitmenu = new SlitMenu(this, screen);
  
  XSetWindowAttributes attrib;
  unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
    CWOverrideRedirect | CWCursor | CWEventMask;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel =
    screen->getBorderColor().pixel;
  attrib.override_redirect = True;
  attrib.cursor = blackbox->getSessionCursor();
  attrib.event_mask = StructureNotifyMask | SubstructureRedirectMask |
    SubstructureNotifyMask | ButtonPressMask;

  frame.x = frame.y = 0;
  frame.width = frame.height = 1;
  
  frame.window =
    XCreateWindow(display, screen->getRootWindow(), frame.x, frame.y,
		  frame.width, frame.height, 0, screen->getDepth(),
		  InputOutput, screen->getVisual(), create_mask, &attrib);

  blackbox->saveSlitSearch(frame.window, this);
  
  reconfigure();
}


Slit::~Slit() {  
  delete clientList;
  delete slitmenu;
  
  screen->getImageControl()->removeImage(frame.pixmap);
  
  XDestroyWindow(display, frame.window);
}


void Slit::addClient(Window w) {
  blackbox->grab();
  
  if (blackbox->validateWindow(w)) {
    SlitClient *client = new SlitClient;
    client->client_window = w;
    
    XWMHints *wmhints = XGetWMHints(display, w);
    
    if (wmhints && (wmhints->flags & IconWindowHint) &&
        (wmhints->icon_window != None)) {
      XMoveWindow(display, client->client_window, screen->getXRes() + 10,
		  screen->getYRes() + 10);
      XMapWindow(display, client->client_window);
      
      client->icon_window = wmhints->icon_window;
      client->window = client->icon_window;
    } else {
      client->icon_window = None;
      client->window = client->client_window;
    }

    XWindowAttributes attrib;  
    if (XGetWindowAttributes(display, client->window, &attrib)) {    
      client->width = attrib.width;
      client->height = attrib.height;
    } else {
      client->width = client->height = 64;
    }

    XSetWindowBorderWidth(display, client->window, 0);

    XSelectInput(display, frame.window, NoEventMask);
    XSelectInput(display, client->window, NoEventMask);

    XReparentWindow(display, client->window, frame.window, 0, 0);
    XChangeSaveSet(display, client->window, SetModeInsert);
      
    XSelectInput(display, frame.window, StructureNotifyMask |
	         SubstructureRedirectMask | SubstructureNotifyMask |
		 ButtonPressMask);
    XSelectInput(display, client->window, StructureNotifyMask |
                 SubstructureNotifyMask);
    XSync(display, False);
    
    clientList->insert(client);
      
    blackbox->saveSlitSearch(client->client_window, this);
    blackbox->saveSlitSearch(client->icon_window, this);
    reconfigure();
  }
  
  blackbox->ungrab();
}


void Slit::removeClient(Window w) {
  blackbox->grab();
  
  LinkedListIterator<SlitClient> it(clientList);
  for (; it.current(); it++)
    if (it.current()->window == w) {
      SlitClient *client = it.current();
      
      blackbox->removeSlitSearch(client->client_window);
      blackbox->removeSlitSearch(client->icon_window);
      clientList->remove(client);
      
      reconfigure();
      
      break;
    }

  blackbox->ungrab();
}


void Slit::reconfigure(void) {
  blackbox->grab();
  
  frame.width = 0;
  frame.height = 0;
  LinkedListIterator<SlitClient> it(clientList);
  for (; it.current(); it++) {
    frame.height += it.current()->height + screen->getBevelWidth();
    
    if (frame.width < it.current()->width)
      frame.width = it.current()->width;
  }
  
  if (frame.width < 1)
    frame.width = 1;
  else
    frame.width += (screen->getBevelWidth() * 2);
  
  if (frame.height < 1)
    frame.height = 1;
  else
    frame.height += screen->getBevelWidth();
  
  // place the slit in the appropriate place
  switch (screen->getSlitPlacement()) {
  case BScreen::TopLeft:
    frame.x = 0;
    frame.y = 0;
    break;

  case BScreen::CenterLeft:
    frame.x = 0;
    frame.y = (screen->getYRes() - frame.height) / 2;
    break;

  case BScreen::BottomLeft:
    frame.x = 0;
    frame.y = screen->getYRes() - frame.height;
    
    if (screen->getToolbar()->getX() < frame.width)
      frame.y -= screen->getToolbar()->getHeight();
    
    break;

  case BScreen::TopRight:
    frame.x = screen->getXRes() - frame.width;
    frame.y = 0;
    break;

  case BScreen::BottomRight:
    frame.x = screen->getXRes() - frame.width;
    frame.y = screen->getYRes() - frame.height;

    if (((signed) (screen->getToolbar()->getX() +
		   screen->getToolbar()->getWidth())) >
	frame.x)
      frame.y -= screen->getToolbar()->getHeight();
    
    break;
    
  case BScreen::CenterRight:
  default:
    frame.x = screen->getXRes() - frame.width;
    frame.y = (screen->getYRes() - frame.height) / 2;
    break;
  }
  
  XMoveResizeWindow(display, frame.window, frame.x, frame.y,
		    frame.width, frame.height);
  
  if (frame.width == 1 || frame.height == 1)
    XUnmapWindow(display, frame.window);
  else
    XMapWindow(display, frame.window);
  
  BImageControl *image_ctrl = screen->getImageControl();
  Pixmap tmp = frame.pixmap;
  frame.pixmap = image_ctrl->
    renderImage(frame.width, frame.height,
		&screen->getTResource()->toolbar.texture);
  if (tmp) image_ctrl->removeImage(tmp);
  
  XSetWindowBackgroundPixmap(display, frame.window, frame.pixmap);
  XClearWindow(display, frame.window);
  
  int x = 0;
  int y = screen->getBevelWidth();
  it.reset();
  for (; it.current(); it++) {
    x = (frame.width - it.current()->width) / 2;
    
    XMoveResizeWindow(display, it.current()->window, x, y,
		      it.current()->width, it.current()->height);
    XMapWindow(display, it.current()->window);

    // for ICCCM compliance

    it.current()->x = x;
    it.current()->y = y;
    
    XEvent event;
    event.type = ConfigureNotify;
  
    event.xconfigure.display = display;
    event.xconfigure.event = it.current()->window;
    event.xconfigure.window = it.current()->window;
    event.xconfigure.x = x;
    event.xconfigure.y = y;
    event.xconfigure.width = it.current()->width;
    event.xconfigure.height = it.current()->height;
    event.xconfigure.border_width = 0;
    event.xconfigure.above = frame.window;
    event.xconfigure.override_redirect = False;
    
    XSendEvent(display, it.current()->window, False, StructureNotifyMask,
	       &event);
    
    y += it.current()->height + screen->getBevelWidth();
  }

  slitmenu->reconfigure();
  
  blackbox->ungrab();
}


void Slit::buttonPressEvent(XButtonEvent *e) {
  blackbox->grab();

  if (e->window == frame.window) {
    int x, y;
    
    switch (screen->getSlitPlacement()) {
    case BScreen::TopLeft:
    case BScreen::CenterLeft:
      x = frame.width;
      y = frame.y;
      break;
      
    case BScreen::BottomLeft:
      x = frame.width;
      y = frame.y + frame.height - slitmenu->getHeight() - 2;
      break;
      
    case BScreen::TopRight:
    case BScreen::CenterRight:
    default:
      x = screen->getXRes() - frame.width - slitmenu->getWidth() - 2;
      y = frame.y;
      break;

    case BScreen::BottomRight:
      x = screen->getXRes() - frame.width - slitmenu->getWidth() - 2;
      y = frame.y + frame.height - slitmenu->getHeight() - 2;
      break;
    }
    
    if (! slitmenu->isVisible()) {
      XRaiseWindow(display, slitmenu->getWindowID());
      
      slitmenu->move(x, y);
      slitmenu->show();
    } else
      slitmenu->hide();
  }

  blackbox->ungrab();
}


void Slit::configureRequestEvent(XConfigureRequestEvent *e) {
  blackbox->grab();
  
  if (blackbox->validateWindow(e->window)) {
    XWindowChanges xwc;
    
    xwc.x = e->x;
    xwc.y = e->y;
    xwc.width = e->width;
    xwc.height = e->height;
    xwc.border_width = 0;
    xwc.sibling = e->above;
    xwc.stack_mode = e->detail;
    
    XConfigureWindow(display, e->window, e->value_mask, &xwc);
    
    LinkedListIterator<SlitClient> it(clientList);
    
    for (; it.current(); it++)
      if (it.current()->window == e->window)
	if (it.current()->width != ((unsigned) e->width) ||
	    it.current()->height != ((unsigned) e->height)) {
	  it.current()->width = (unsigned) e->width;
	  it.current()->height = (unsigned) e->height;
	  
	  reconfigure();
	  
	  break;
	}
  }
  
  blackbox->ungrab();
}


SlitMenu::SlitMenu(Slit *sl, BScreen *scr) :
  Basemenu(scr->getBlackbox(), scr)
{
  slit = sl;
  screen = scr;
  
  setTitleVisibility(False);
  setMovable(False);
  setHidable(True);

  defaultMenu();

  insert("Top Left");
  insert("Center Left");
  insert("Bottom Left");
  insert("Top Right");
  insert("Center Right");
  insert("Bottom Right");

  update();
}


void SlitMenu::itemSelected(int button, int item) {
  if (button == 1) {
    screen->saveSlitPlacement(item + 1);

    hide();

    slit->reconfigure();
  }
}


#endif

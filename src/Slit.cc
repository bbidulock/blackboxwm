// Slit.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software. 
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
// DEALINGS IN THE SOFTWARE.
  
// stupid macros needed to access some functions in version 2 of the GNU C 
// library
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include <X11/keysym.h>

#ifdef    SLIT

#include "blackbox.hh"
#include "Image.hh"
#include "Screen.hh"
#include "Slit.hh"
#include "Toolbar.hh"

#ifdef    DEBUG
#  include "mem.h"
#endif // DEBUG


Slit::Slit(BScreen *scr) {
#ifdef    DEBUG
  allocate(sizeof(Slit), "Slit.cc");
#endif // DEBUG
  
  screen = scr;
  blackbox = screen->getBlackbox();

  on_top = screen->isSlitOnTop();

  display = screen->getBaseDisplay()->getXDisplay();
  frame.window = frame.pixmap = None;

  clientList = new LinkedList<SlitClient>;

  slitmenu = new Slitmenu(this);
  
  XSetWindowAttributes attrib;
  unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
    CWOverrideRedirect | CWEventMask;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel =
    screen->getBorderColor()->getPixel();
  attrib.override_redirect = True;
  attrib.event_mask = SubstructureRedirectMask | ButtonPressMask;

  frame.x = frame.y = 0;
  frame.width = frame.height = 1;
  
  frame.window =
    XCreateWindow(display, screen->getRootWindow(), frame.x, frame.y,
		  frame.width, frame.height, screen->getBorderWidth(),
                  screen->getDepth(), InputOutput, screen->getVisual(),
                  create_mask, &attrib);
  blackbox->saveSlitSearch(frame.window, this);

  reconfigure();
}


Slit::~Slit() {
#ifdef    DEBUG
  deallocate(sizeof(Slit), "Slit.cc");
#endif // DEBUG

  blackbox->grab();

  while (clientList->count()) {
    SlitClient *client = clientList->find(0);

    XSelectInput(display, client->window, NoEventMask);
    XReparentWindow(display, client->window, screen->getRootWindow(),
		    frame.x + client->x, frame.y + client->y);
    XMapWindow(display, client->window);

    removeClient(client->window);
  }

  XFlush(display);
  
  delete clientList;
  delete slitmenu;
  
  screen->getImageControl()->removeImage(frame.pixmap);
  
  blackbox->removeSlitSearch(frame.window);
  
  XDestroyWindow(display, frame.window);

  blackbox->ungrab();
}


void Slit::addClient(Window w) {
  blackbox->grab();
  
  if (blackbox->validateWindow(w)) {
    SlitClient *client = new SlitClient;
    client->client_window = w;
    
    XWMHints *wmhints = XGetWMHints(display, w);
    
    if (wmhints) {
      if ((wmhints->flags & IconWindowHint) &&
	  (wmhints->icon_window != None)) {
	XMoveWindow(display, client->client_window, screen->getWidth() + 10,
		    screen->getHeight() + 10);
	XMapWindow(display, client->client_window);
	
	client->icon_window = wmhints->icon_window;
	client->window = client->icon_window;
      } else {
	client->icon_window = None;
	client->window = client->client_window;
      }
      
      XFree(wmhints);
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
    XMapRaised(display, client->window);
    XChangeSaveSet(display, client->window, SetModeInsert);
      
    XSelectInput(display, frame.window, SubstructureRedirectMask |
		 ButtonPressMask);
    XSelectInput(display, client->window, StructureNotifyMask |
                 SubstructureNotifyMask);
    XFlush(display);
    
    clientList->insert(client);
      
    blackbox->saveSlitSearch(client->client_window, this);
    blackbox->saveSlitSearch(client->icon_window, this);
    reconfigure();
  }
  
  blackbox->ungrab();
}


void Slit::removeClient(Window w, Bool remap) {
  blackbox->grab();

  Bool reconf = False;
  
  LinkedListIterator<SlitClient> it(clientList);
  for (; it.current(); it++)
    if (it.current()->window == w) {
      SlitClient *client = it.current();
      
      blackbox->removeSlitSearch(client->client_window);
      blackbox->removeSlitSearch(client->icon_window);
      clientList->remove(client);
      screen->removeNetizen(client->window);
	
      if (remap && blackbox->validateWindow(w)) {
        XSelectInput(display, frame.window, NoEventMask);
        XSelectInput(display, client->window, NoEventMask);
        XReparentWindow(display, client->window, screen->getRootWindow(),
                        client->x, client->y);
        XUnmapWindow(display, client->window);
        XChangeSaveSet(display, client->window, SetModeDelete);
        XSelectInput(display, frame.window, SubstructureRedirectMask |
                     ButtonPressMask);
        XFlush(display);
      }

      delete client;
      reconf = True;

      break;
    }

  if (reconf) reconfigure();

  blackbox->ungrab();
}


void Slit::reconfigure(void) {
  frame.width = 0;
  frame.height = 0;
  LinkedListIterator<SlitClient> it(clientList);

  switch (screen->getSlitDirection()) {
  case Vertical:
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

    break;

  case Horizontal:
    for (; it.current(); it++) {
      frame.width += it.current()->width + screen->getBevelWidth();

      if (frame.height < it.current()->height)
        frame.height = it.current()->height;
    }

    if (frame.width < 1)
      frame.width = 1;
    else
      frame.width += screen->getBevelWidth();
 
    if (frame.height < 1)
      frame.height = 1;
    else
      frame.height += (screen->getBevelWidth() * 2);

    break;
  }
  
  reposition();

  XSetWindowBorderWidth(display ,frame.window, screen->getBorderWidth());
  XSetWindowBorder(display, frame.window,
                   screen->getBorderColor()->getPixel());
  
  if (! clientList->count())
    XUnmapWindow(display, frame.window);
  else
    XMapWindow(display, frame.window);
  
  BImageControl *image_ctrl = screen->getImageControl();
  Pixmap tmp = frame.pixmap;
  frame.pixmap = image_ctrl->
    renderImage(frame.width, frame.height,
		&screen->getToolbarStyle()->toolbar);
  if (tmp) image_ctrl->removeImage(tmp);
  
  XSetWindowBackgroundPixmap(display, frame.window, frame.pixmap);
  XClearWindow(display, frame.window);
  
  int x, y;
  it.reset();

  switch (screen->getSlitDirection()) {
  case Vertical:
    x = 0;
    y = screen->getBevelWidth();

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

    break;

  case Horizontal:
    x = screen->getBevelWidth();
    y = 0;

    for (; it.current(); it++) {
      y = (frame.height - it.current()->height) / 2;

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
      event.xconfigure.x = frame.x + x + screen->getBorderWidth();
      event.xconfigure.y = frame.y + y + screen->getBorderWidth();
      event.xconfigure.width = it.current()->width;
      event.xconfigure.height = it.current()->height;
      event.xconfigure.border_width = 0;
      event.xconfigure.above = frame.window;
      event.xconfigure.override_redirect = False;

      XSendEvent(display, it.current()->window, False, StructureNotifyMask,
                 &event);

      x += it.current()->width + screen->getBevelWidth();
    }

    break;
  }

  slitmenu->reconfigure();
}


void Slit::reposition(void) {
  // place the slit in the appropriate place
  switch (screen->getSlitPlacement()) {
  case TopLeft:
    frame.x = 0;
    frame.y = 0;
    break;
    
  case CenterLeft:
    frame.x = 0;
    frame.y = (screen->getHeight() - frame.height) / 2;
    break;
    
  case BottomLeft:
    frame.x = 0;
    frame.y = screen->getHeight() - frame.height - screen->getBorderWidth2x(); 
    break;
  
  case TopCenter:
    frame.x = (screen->getWidth() - frame.width) / 2;
    frame.y = 0;
    break;
  
  case BottomCenter:
    frame.x = (screen->getWidth() - frame.width) / 2;
    frame.y = screen->getHeight() - frame.height - screen->getBorderWidth2x();
    break;
    
  case TopRight:
    frame.x = screen->getWidth() - frame.width;
    frame.y = 0;
    break;
      
  case BottomRight:
    frame.x = screen->getWidth() - frame.width;
    frame.y = screen->getHeight() - frame.height - screen->getBorderWidth2x();
    break;
    
  case CenterRight:
  default:
    frame.x = screen->getWidth() - frame.width;
    frame.y = (screen->getHeight() - frame.height) / 2;
    break;
  } 
 
  int sw = frame.width + screen->getBorderWidth2x(),
    sh = frame.height + screen->getBorderWidth2x(),
    tw = screen->getToolbar()->getWidth() + screen->getBorderWidth2x(),
    th = screen->getToolbar()->getHeight() + screen->getBorderWidth2x();
    
  if (screen->getToolbar()->getX() < frame.x + sw &&
      screen->getToolbar()->getX() + tw > frame.x &&
      screen->getToolbar()->getY() < frame.y + sh &&
      screen->getToolbar()->getY() + th > frame.y) {
    if (frame.y < th) frame.y += th;
    else frame.y -= th;
  }
  
  XMoveResizeWindow(display, frame.window, frame.x, frame.y,
                    frame.width, frame.height);
}


void Slit::buttonPressEvent(XButtonEvent *e) {
  if (e->window != frame.window) return;

  if (e->button == Button1 && (! on_top)) {
    Window w[1] = { frame.window };
    screen->raiseWindows(w, 1);
  } else if (e->button == Button2 && (! on_top)) {
    XLowerWindow(display, frame.window);
  } else if (e->button == Button3) {
    if (! slitmenu->isVisible()) {
      int x, y;

      x = e->x_root - (slitmenu->getWidth() / 2);
      y = e->y_root - (slitmenu->getHeight() / 2);

      if (x < 0)
        x = 0;
      else if (x + slitmenu->getWidth() > screen->getWidth())
        x = screen->getWidth() - slitmenu->getWidth();

      if (y < 0)
        y = 0;
      else if (y + slitmenu->getHeight() > screen->getHeight()) 
        y = screen->getHeight() - slitmenu->getHeight();
 
      slitmenu->move(x, y);
      slitmenu->show();
    } else
      slitmenu->hide();
  }
}


void Slit::configureRequestEvent(XConfigureRequestEvent *e) {
  blackbox->grab();
  
  if (blackbox->validateWindow(e->window)) {
    Bool reconf = False;
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

          reconf = True;

          break;
        }
	  
    if (reconf) reconfigure();
	  
  }
  
  blackbox->ungrab();
}


Slitmenu::Slitmenu(Slit *sl) : Basemenu(sl->screen) {
  slit = sl;

  setLabel("Slit");
  setInternalMenu();

  directionmenu = new Directionmenu(this);
  placementmenu = new Placementmenu(this);

  insert("Direction", directionmenu);
  insert("Placement", placementmenu);
  insert("Always on top", 1);
  
  update();

  if (slit->isOnTop()) setItemSelected(2, True);
}


Slitmenu::~Slitmenu(void) {
  delete directionmenu;
  delete placementmenu;
}


void Slitmenu::itemSelected(int button, int index) {
  if (button == 1) {
    BasemenuItem *item = find(index);
    if (! item) return;

    if (item->function() == 1) {
      Bool change = ((slit->isOnTop()) ?  False : True);
      slit->on_top = change;
      setItemSelected(2, change);

      if (slit->isOnTop()) slit->screen->raiseWindows((Window *) 0, 0);
    }
  }
}


void Slitmenu::reconfigure(void) {
  directionmenu->reconfigure();
  placementmenu->reconfigure();

  Basemenu::reconfigure();
}


Slitmenu::Directionmenu::Directionmenu(Slitmenu *sm) : Basemenu(sm->slit->screen) {
#ifdef    DEBUG
  allocate(sizeof(Directionmenu), "Slit.cc");
#endif // DEBUG
  
  slitmenu = sm;

  setLabel("Slit Direction");
  setInternalMenu();

  insert("Horizontal", Slit::Horizontal);
  insert("Vertical", Slit::Vertical);

  update();

  if (sm->slit->screen->getSlitDirection() == Slit::Horizontal)
    setItemSelected(0, True);
  else
    setItemSelected(1, True);
}


#ifdef    DEBUG
Slitmenu::Directionmenu::~Directionmenu(void) {
  deallocate(sizeof(Directionmenu), "Slit.cc");
}
#endif // DEBUG


void Slitmenu::Directionmenu::itemSelected(int button, int index) {
  if (button == 1) {
    BasemenuItem *item = find(index);
    if (! item) return;

    slitmenu->slit->screen->saveSlitDirection(item->function());

    if (item->function() == Slit::Horizontal) {
      setItemSelected(0, True);
      setItemSelected(1, False);
    } else {
      setItemSelected(0, False);
      setItemSelected(1, True);
    }

    hide();
    slitmenu->slit->reconfigure();
  }
}


Slitmenu::Placementmenu::Placementmenu(Slitmenu *sm) : Basemenu(sm->slit->screen) {
#ifdef   DEBUG
  allocate(sizeof(Placementmenu), "Slit.cc");
#endif // DEBUG

  slitmenu = sm;
  
  setLabel("Slit Placement");
  setMinimumSublevels(3);
  setInternalMenu();

  insert("Top Left", Slit::TopLeft);
  insert("Center Left", Slit::CenterLeft);
  insert("Bottom Left", Slit::BottomLeft);
  insert("Top Center", Slit::TopCenter);
  insert("");
  insert("Bottom Center", Slit::BottomCenter);
  insert("Top Right", Slit::TopRight);
  insert("Center Right", Slit::CenterRight);
  insert("Bottom Right", Slit::BottomRight);

  update();
}


#ifdef    DEBUG
Slitmenu::Placementmenu::~Placementmenu(void) {
  deallocate(sizeof(Placementmenu), "Slit.cc");
}
#endif // DEBUG


void Slitmenu::Placementmenu::itemSelected(int button, int index) {
  if (button == 1) {
    BasemenuItem *item = find(index);
    if (! item) return;

    if (item->function()) {
      slitmenu->slit->screen->saveSlitPlacement(item->function());
      hide();
      slitmenu->slit->reconfigure();
    }
  }
}


#ifdef    DEBUG
Slit::SlitClient::SlitClient(void) {
  allocate(sizeof(SlitClient), "Slit.cc");
}


Slit::SlitClient::~SlitClient(void) {
  deallocate(sizeof(SlitClient), "Slit.cc");
}
#endif // DEBUG

#endif // SLIT

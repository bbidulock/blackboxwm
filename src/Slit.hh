// Slit.hh for Blackbox - an X11 Window manager
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

#ifndef   __Slit_hh
#define   __Slit_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Basemenu.hh"
#include "LinkedList.hh"

class Slit;


class SlitClient {
public:
  Window client_window, icon_window, window;
  
  int x, y;
  unsigned int width, height;
};


class Slitmenu : public Basemenu {
private:
  BScreen *screen;
  Slit *slit;
  
  
protected:
  virtual void itemSelected(int, int);
  
  
public:
  Slitmenu(Slit *, BScreen *);
};


class Slit {
private:
  Display *display;
  
  Blackbox *blackbox;
  BScreen *screen;
  LinkedList<SlitClient> *clientList;
  Slitmenu *slitmenu;
  
  struct frame {
    Pixmap pixmap;
    Window window;
    
    int x, y;
    unsigned int width, height;
  } frame;
  
  
protected:
  
  
public:
  Slit(Blackbox *, BScreen *);
  ~Slit();

  Slitmenu *getMenu() { return slitmenu; }

  Window getWindowID() { return frame.window; }
  
  void addClient(Window);
  void removeClient(Window);
  void reconfigure(void);

  void buttonPressEvent(XButtonEvent *);
  void configureRequestEvent(XConfigureRequestEvent *);
};


#endif // __Slit_hh

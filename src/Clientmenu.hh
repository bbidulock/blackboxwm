//
// Clientmenu.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
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

#ifndef __Clientmenu_hh
#define __Clientmenu_hh

// forward declarations
class Clientmenu;

class Blackbox;
class Workspace;

#include "Basemenu.hh"


#ifdef __Basemenu_hh

class Clientmenu : public Basemenu {
private:
  BScreen *screen;
  Workspace *wkspc;


protected:
  virtual void itemSelected(int, int);


public:
  Clientmenu(Blackbox *, Workspace *);
};

#endif // __Basemenu_hh


#endif // __Clientmenu_hh

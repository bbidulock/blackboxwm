//
// blackbox.hh for Blackbox - an X11 Window manager
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

#ifndef _blackbox_hh
#define _blackbox_hh
#define _blackbox_version "zero point two zero point zero beta"

#include <X11/Xlib.h>

#include "llist.hh"


// forward declaration
class BlackboxSession;


class Blackbox {
private:
  llist<BlackboxSession> *session_list;

  char **b_argv;
  int b_argc;


protected:


public:
  Blackbox(int, char **);
  ~Blackbox(void);

  void EventLoop(void);
  void Restart(char * = 0);
  void Shutdown(Bool = True);
};


extern Blackbox *blackbox;


#endif

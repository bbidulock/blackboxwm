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
#define _blackbox_version "zero point one one point three beta"

//
//   This is a class to manage a single X server connection.  It allows for
//   future expansion and plans to manage multiple X server connections.
//
class BlackboxSession;

#include "debug.hh"
#include "llist.hh"


class Blackbox {
public:
  Blackbox(int, char **);
  ~Blackbox(void);

  void EventLoop(void);
  void Restart(char * = 0);
protected:

private:
  BlackboxSession *session;
  Debugger *debug;
  llist<BlackboxSession> *session_list;

  char **b_argv;
  int b_argc;
};

extern Blackbox *blackbox;


#endif

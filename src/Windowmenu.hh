// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Windowmenu.hh for Blackbox - an X11 window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2003
//         Bradley T Hughes <bhughes at trolltech.com>
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

#ifndef   __Windowmenu_hh
#define   __Windowmenu_hh

#include <Menu.hh>

// forward declarations
class SendToWorkspacemenu;
class BlackboxWindow;

class Windowmenu : public bt::Menu {
public:
  Windowmenu(bt::Application &app, unsigned int screen);

  void setWindow(BlackboxWindow *win);
  void hide(void);
  void refresh(void);

protected:
  virtual void itemClicked(unsigned int id, unsigned int);

private:
  BlackboxWindow *_window;
  SendToWorkspacemenu *_sendto;

  friend class SendtoWorkspacemenu;
};

#endif // __Windowmenu_hh

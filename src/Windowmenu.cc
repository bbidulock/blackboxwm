// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Windowmenu.cc for Blackbox - an X11 window manager
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

#include "Windowmenu.hh"
#include "Screen.hh"
#include "Window.hh"
#include "../nls/blackbox-nls.hh"

#include <i18n.hh>

#include <assert.h>


class SendToWorkspacemenu : public bt::Menu {
public:
  SendToWorkspacemenu(bt::Application &app, unsigned int screen);

  inline void setWindow(BlackboxWindow *win)
  { _window = win; }

  void refresh(void);

protected:
  virtual void itemClicked(unsigned int id, unsigned int button);

private:
  BlackboxWindow *_window;
};


enum {
  SendTo,
  Shade,
  Iconify,
  Maximize,
  Raise,
  Lower,
  KillClient,
  Close
};

Windowmenu::Windowmenu(bt::Application &app, unsigned int screen)
  : bt::Menu(app, screen), _window(0) {
  _sendto = new SendToWorkspacemenu(app, screen);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuSendTo, "Send To ..."),
             _sendto, SendTo);
  insertSeparator();
  insertItem(bt::i18n(WindowmenuSet, WindowmenuShade, "Shade"),
             Shade);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuIconify, "Iconify"),
             Iconify);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuMaximize, "Maximize"),
             Maximize);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuRaise, "Raise"),
             Raise);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuLower, "Lower"),
             Lower);
  insertSeparator();
  insertItem(bt::i18n(WindowmenuSet, WindowmenuKillClient, "Kill Client"),
             KillClient);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuClose, "Close"),
             Close);
}


void Windowmenu::setWindow(BlackboxWindow *win) {
  _window = win;
  _sendto->setWindow(win);
}


void Windowmenu::hide(void) {
  bt::Menu::hide();
  setWindow(0);
}


void Windowmenu::refresh(void) {
  assert(_window != 0);

  setItemEnabled(Shade, _window->hasWindowFunction(WindowFunctionShade));
  setItemChecked(Shade, _window->isShaded());

  setItemEnabled(Maximize, _window->hasWindowFunction(WindowFunctionMaximize));
  setItemChecked(Maximize, _window->isMaximized());

  setItemEnabled(Iconify, _window->hasWindowFunction(WindowFunctionIconify));
  setItemEnabled(Close, _window->hasWindowFunction(WindowFunctionClose));
}


void Windowmenu::itemClicked(unsigned int id, unsigned int) {
  switch (id) {
  case Shade:
    _window->shade();
    break;

  case Iconify:
    _window->iconify();
    break;

  case Maximize:
    _window->maximize(1);
    break;

  case Close:
    _window->close();
    break;

  case Raise: {
    _window->getScreen()->raiseWindow(_window);
    break;
  }

  case Lower: {
    _window->getScreen()->lowerWindow(_window);
    break;
  }

  case KillClient:
    XKillClient(_window->getScreen()->screenInfo().display().XDisplay(),
                _window->getClientWindow());
    break;
  } // switch
}


SendToWorkspacemenu::SendToWorkspacemenu(bt::Application &app,
                                         unsigned int screen)
  : bt::Menu(app, screen), _window(0)
{ }


void SendToWorkspacemenu::refresh(void) {
  assert(_window != 0);

  clear();
  const unsigned num = _window->getScreen()->resource().numberOfWorkspaces();
  for (unsigned int i = 0; i < num; ++i)
    insertItem(_window->getScreen()->resource().nameOfWorkspace(i), i);

  /*
    give a little visual indication to the user about which workspace
    the window is on.  you obviously can't send a window to the workspace
    the window is already on, which is why it is checked and disabled
  */
  setItemEnabled(_window->workspace(), false);
  setItemChecked(_window->workspace(), true);
}


void SendToWorkspacemenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 2) _window->withdraw();
  _window->getScreen()->reassociateWindow(_window, id);
  if (button == 2) _window->getScreen()->setCurrentWorkspace(id);
}

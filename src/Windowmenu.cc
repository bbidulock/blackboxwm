// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
//
// Blackbox - an X11 window manager
//
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 Bradley T Hughes <bhughes at trolltech.com>
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "Windowmenu.hh"
#include "Window.hh"
#include "i18n.hh"


class SendToWorkspacemenu : public bt::Menu {
public:
  SendToWorkspacemenu(bt::Application &app, unsigned int screen,
                      BlackboxWindow *window);

  void refresh(void);

protected:
  virtual void itemClicked(unsigned int id, unsigned int button);

private:
  BlackboxWindow *_window;
};


Windowmenu::Windowmenu(bt::Application &app, unsigned int screen,
                       BlackboxWindow *window)
  : bt::Menu(app, screen), _window(window) {
  _sendto = new SendToWorkspacemenu(app, screen, _window);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuSendTo, "Send To ..."),
             _sendto);
  insertSeparator();
  insertItem(bt::i18n(WindowmenuSet, WindowmenuShade, "Shade"),
             BScreen::WindowShade);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuIconify, "Iconify"),
             BScreen::WindowIconify);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuMaximize, "Maximize"),
             BScreen::WindowMaximize);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuRaise, "Raise"),
             BScreen::WindowRaise);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuLower, "Lower"),
             BScreen::WindowLower);
  insertSeparator();
  insertItem(bt::i18n(WindowmenuSet, WindowmenuKillClient, "Kill Client"),
             BScreen::WindowKill);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuClose, "Close"),
             BScreen::WindowClose);
}


void Windowmenu::refresh(void) {
  setItemEnabled(BScreen::WindowShade, _window->hasTitlebar());
  setItemChecked(BScreen::WindowShade, _window->isShaded());

  setItemEnabled(BScreen::WindowMaximize, _window->isMaximizable());
  setItemChecked(BScreen::WindowMaximize, _window->isMaximized());

  setItemEnabled(BScreen::WindowIconify, _window->isIconifiable());
  setItemEnabled(BScreen::WindowClose, _window->isClosable());
}


void Windowmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  switch (id) {
  case BScreen::WindowShade:
    _window->shade();
    break;

  case BScreen::WindowIconify:
    _window->iconify();
    break;

  case BScreen::WindowMaximize:
    _window->maximize(button);
    break;

  case BScreen::WindowClose:
    _window->close();
    break;

  case BScreen::WindowRaise: {
    _window->getScreen()->raiseWindow(_window);
    break;
  }

  case BScreen::WindowLower: {
    _window->getScreen()->lowerWindow(_window);
    break;
  }

  case BScreen::WindowKill:
    XKillClient(_window->getScreen()->getScreenInfo().getDisplay().XDisplay(),
                _window->getClientWindow());
    break;
  } // switch
}


SendToWorkspacemenu::SendToWorkspacemenu(bt::Application &app,
                                         unsigned int screen,
                                         BlackboxWindow *window)
  : bt::Menu(app, screen), _window(window) { }


void SendToWorkspacemenu::refresh(void) {
  clear();
  unsigned workspace_count = _window->getScreen()->getWorkspaceCount();
  for (unsigned int i = 0; i < workspace_count; ++i)
    insertItem(_window->getScreen()->getWorkspaceName(i), i);

  /*
    give a little visual indication to the user about which workspace
    the window is on.  you obviously can't send a window to that
    workspace the window is already on, which is why it is checked and
    disabled
  */
  setItemEnabled(_window->getWorkspaceNumber(), false);
  setItemChecked(_window->getWorkspaceNumber(), true);
}


void SendToWorkspacemenu::itemClicked(unsigned int id, unsigned int button) {
  if (button > 2) return;

  if (button == 1) _window->withdraw();
  _window->getScreen()->reassociateWindow(_window, id);
  if (button == 2) _window->getScreen()->changeWorkspaceID(id);
}

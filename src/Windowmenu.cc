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


enum WindowAction {
  WindowActionShade,
  WindowActionIconify,
  WindowActionMaximize,
  WindowActionRaise,
  WindowActionLower,
  WindowActionKillClient,
  WindowActionClose
};

Windowmenu::Windowmenu(bt::Application &app, unsigned int screen,
                       BlackboxWindow *window)
  : bt::Menu(app, screen), _window(window) {
  _sendto = new SendToWorkspacemenu(app, screen, _window);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuSendTo, "Send To ..."),
             _sendto);
  insertSeparator();
  insertItem(bt::i18n(WindowmenuSet, WindowmenuShade, "Shade"),
             WindowActionShade);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuIconify, "Iconify"),
             WindowActionIconify);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuMaximize, "Maximize"),
             WindowActionMaximize);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuRaise, "Raise"),
             WindowActionRaise);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuLower, "Lower"),
             WindowActionLower);
  insertSeparator();
  insertItem(bt::i18n(WindowmenuSet, WindowmenuKillClient, "Kill Client"),
             WindowActionKillClient);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuClose, "Close"),
             WindowActionClose);
}


void Windowmenu::refresh(void) {
  setItemEnabled(WindowActionShade,
                 _window->hasFunction(BlackboxWindow::Func_Shade));
  setItemChecked(WindowActionShade, _window->isShaded());

  setItemEnabled(WindowActionMaximize,
                 _window->hasFunction(BlackboxWindow::Func_Maximize));
  setItemChecked(WindowActionMaximize, _window->isMaximized());

  setItemEnabled(WindowActionIconify,
                 _window->hasFunction(BlackboxWindow::Func_Iconify));
  setItemEnabled(WindowActionClose,
                 _window->hasFunction(BlackboxWindow::Func_Close));
}


void Windowmenu::itemClicked(unsigned int id, unsigned int) {
  switch (id) {
  case WindowActionShade:
    _window->shade();
    break;

  case WindowActionIconify:
    _window->iconify();
    break;

  case WindowActionMaximize:
    _window->maximize(1);
    break;

  case WindowActionClose:
    _window->close();
    break;

  case WindowActionRaise: {
    _window->getScreen()->raiseWindow(_window);
    break;
  }

  case WindowActionLower: {
    _window->getScreen()->lowerWindow(_window);
    break;
  }

  case WindowActionKillClient:
    XKillClient(_window->getScreen()->screenInfo().display().XDisplay(),
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
  const unsigned num = _window->getScreen()->resource().numberOfWorkspaces();
  for (unsigned int i = 0; i < num; ++i)
    insertItem(_window->getScreen()->resource().nameOfWorkspace(i), i);

  /*
    give a little visual indication to the user about which workspace
    the window is on.  you obviously can't send a window to the workspace
    the window is already on, which is why it is checked and disabled
  */
  setItemEnabled(_window->getWorkspaceNumber(), false);
  setItemChecked(_window->getWorkspaceNumber(), true);
}


void SendToWorkspacemenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 2) _window->withdraw();
  _window->getScreen()->reassociateWindow(_window, id);
  if (button == 2) _window->getScreen()->changeWorkspaceID(id);
}

// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Windowmenu.cc for Blackbox - an X11 window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2005
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
#include "Workspace.hh"

#include <Unicode.hh>

#include <assert.h>

// #define WITH_FULLSCREEN


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
  SendToWorkspace,
  OccupyAllWorkspaces,
  Shade,
  Iconify,
  Maximize,
#if defined(WITH_FULLSCREEN)
  FullScreen,
#endif
  AlwaysOnTop,
  AlwaysOnBottom,
  KillClient,
  Close
};

Windowmenu::Windowmenu(bt::Application &app, unsigned int screen)
  : bt::Menu(app, screen), _window(0)
{
  _sendto = new SendToWorkspacemenu(app, screen);
  insertItem(bt::toUnicode("Send to Workspace"), _sendto, SendToWorkspace);
  insertItem(bt::toUnicode("Occupy all Workspaces"), OccupyAllWorkspaces);
  insertSeparator();
  insertItem(bt::toUnicode("Shade"), Shade);
  insertItem(bt::toUnicode("Iconify"), Iconify);
  insertItem(bt::toUnicode("Maximize"), Maximize);
#if defined(WITH_FULLSCREEN)
  insertItem(bt::toUnicode("Full Screen"), FullScreen);
#endif
  insertItem(bt::toUnicode("Always on top"), AlwaysOnTop);
  insertItem(bt::toUnicode("Always on bottom"), AlwaysOnBottom);
  insertSeparator();
  insertItem(bt::toUnicode("Kill Client"), KillClient);
  insertItem(bt::toUnicode("Close"), Close);
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

  setItemEnabled(SendToWorkspace,
                 _window->hasWindowFunction(WindowFunctionChangeWorkspace)
                 && _window->workspace() != bt::BSENTINEL);

  setItemEnabled(OccupyAllWorkspaces,
                 _window->hasWindowFunction(WindowFunctionChangeWorkspace));
  setItemChecked(OccupyAllWorkspaces,
                 _window->workspace() == bt::BSENTINEL);

  setItemEnabled(Shade, _window->hasWindowFunction(WindowFunctionShade));
  setItemChecked(Shade, _window->isShaded());

  setItemEnabled(Iconify, _window->hasWindowFunction(WindowFunctionIconify));
  setItemChecked(Iconify, _window->isIconic());

  setItemEnabled(Maximize, _window->hasWindowFunction(WindowFunctionMaximize));
  setItemChecked(Maximize, _window->isMaximized());

#if defined(WITH_FULLSCREEN)
  setItemEnabled(FullScreen, _window->windowType() == WindowTypeNormal);
  setItemChecked(FullScreen, _window->isFullScreen());
#endif

  setItemEnabled(AlwaysOnTop,
                 _window->hasWindowFunction(WindowFunctionChangeLayer));
  setItemChecked(AlwaysOnTop,
                 _window->layer() == StackingList::LayerAbove);

  setItemEnabled(AlwaysOnBottom,
                 _window->hasWindowFunction(WindowFunctionChangeLayer));
  setItemChecked(AlwaysOnBottom,
                 _window->layer() == StackingList::LayerBelow);

  setItemEnabled(KillClient, !_window->isTransient());

  setItemEnabled(Close, _window->hasWindowFunction(WindowFunctionClose));
}


void Windowmenu::itemClicked(unsigned int id, unsigned int) {
  switch (id) {
  case OccupyAllWorkspaces:
    {
      BScreen *screen = _window->screen();
      unsigned int workspace = (_window->workspace() == bt::BSENTINEL
                                ? screen->currentWorkspace()
                                : bt::BSENTINEL);
      _window->changeWorkspace(workspace);
      break;
    }

  case Shade:
    _window->setShaded(!_window->isShaded());
    break;

  case Iconify:
    _window->iconify();
    break;

  case Maximize:
    _window->maximize(1);
    break;

#if defined(WITH_FULLSCREEN)
  case FullScreen:
    _window->setFullScreen(!_window->isFullScreen());
    break;
#endif

  case AlwaysOnTop:
    {
      StackingList::Layer new_layer =
        (_window->layer() == StackingList::LayerAbove
         ? StackingList::LayerNormal
         : StackingList::LayerAbove);
      _window->changeLayer(new_layer);
      break;
    }

  case AlwaysOnBottom:
    {
      StackingList::Layer new_layer =
        (_window->layer() == StackingList::LayerBelow
         ? StackingList::LayerNormal
         : StackingList::LayerBelow);
      _window->changeLayer(new_layer);
      break;
    }

  case KillClient:
    XKillClient(_window->screen()->screenInfo().display().XDisplay(),
                _window->clientWindow());
    break;

  case Close:
    _window->close();
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
  BScreen *screen = _window->screen();
  const unsigned num = screen->workspaceCount();
  for (unsigned int i = 0; i < num; ++i)
    insertItem(screen->resource().workspaceName(i), i);

  /*
    give a little visual indication to the user about which workspace
    the window is on.  you obviously can't send a window to the workspace
    the window is already on, which is why it is checked and disabled
  */
  setItemEnabled(_window->workspace(), false);
  setItemChecked(_window->workspace(), true);
}


void SendToWorkspacemenu::itemClicked(unsigned int id, unsigned int button) {
  assert(_window != 0);
  _window->changeWorkspace(id,
                           (button == 2
                            ? BlackboxWindow::SwitchToNewWorkspace
                            : BlackboxWindow::StayOnCurrentWorkspace));

  if (button == 2) {
    BScreen *screen = _window->screen();
    screen->setCurrentWorkspace(_window->workspace());
  }
}

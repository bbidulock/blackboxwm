// Windowmenu.cc for Blackbox - an X11 Window manager
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

#include "i18n.hh"
#include "blackbox.hh"
#include "Screen.hh"
#include "Window.hh"
#include "Windowmenu.hh"
#include "Workspace.hh"

#ifdef    DEBUG
#  include "mem.h"
#endif // DEBUG

#ifdef    STDC_HEADERS
#  include <string.h>
#endif // STDC_HEADERS


Windowmenu::Windowmenu(BlackboxWindow *win) : Basemenu(win->getScreen()) {
#ifdef    DEBUG
  allocate(sizeof(Windowmenu), "Windowmenu.cc");
#endif // DEBUG

  window = win;
  screen = window->getScreen();

  setTitleVisibility(False);
  setMovable(False);
  setInternalMenu();

  sendToMenu = new SendtoWorkspacemenu(this);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuSendTo,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Send To ..."),
	 sendToMenu);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuShade,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Shade"),
	 BScreen::WindowShade);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuIconify,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Iconify"),
	 BScreen::WindowIconify);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuMaximize,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Maximize"),
	 BScreen::WindowMaximize);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuRaise,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Raise"),
	 BScreen::WindowRaise);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuLower,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Lower"),
	 BScreen::WindowLower);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuStick,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Stick"),
	 BScreen::WindowStick);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuKillClient,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Kill Client"),
	 BScreen::WindowKill);
  insert(i18n->getMessage(
#ifdef    NLS
			  WindowmenuSet, WindowmenuClose,
#else // !NLS
			  0, 0,
#endif // NLS
			  "Close"),
	 BScreen::WindowClose);

  update();

  setItemEnabled(1, window->hasTitlebar());
  setItemEnabled(2, window->isIconifiable());
  setItemEnabled(3, window->isMaximizable());
  setItemEnabled(8, window->isClosable());
}


Windowmenu::~Windowmenu(void) {
#ifdef    DEBUG
  deallocate(sizeof(Windowmenu), "Windowmenu.cc");
#endif // DEBUG

  delete sendToMenu;
}


void Windowmenu::show(void) {
  if (isItemEnabled(1)) setItemSelected(1, window->isShaded());
  if (isItemEnabled(3)) setItemSelected(3, window->isMaximized());
  if (isItemEnabled(6)) setItemSelected(6, window->isStuck());

  Basemenu::show();
}


void Windowmenu::itemSelected(int button, int index) {
  BasemenuItem *item = find(index);

  switch (item->function()) {
  case BScreen::WindowShade:
    hide();
    window->shade();
    break;

  case BScreen::WindowIconify:
    hide();
    window->iconify();
    break;

  case BScreen::WindowMaximize:
    hide();
    window->maximize((unsigned int) button);
    break;

  case BScreen::WindowClose:
    hide();
    window->close();
    break;

  case BScreen::WindowRaise:
    hide();
    screen->getWorkspace(window->getWorkspaceNumber())->raiseWindow(window);
    break;

  case BScreen::WindowLower:
    hide();
    screen->getWorkspace(window->getWorkspaceNumber())->lowerWindow(window);
    break;

  case BScreen::WindowStick:
    hide();
    window->stick();
    break;

  case BScreen::WindowKill:
    hide();
    XKillClient(screen->getBaseDisplay()->getXDisplay(),
                window->getClientWindow());
    break;
  }
}


void Windowmenu::reconfigure(void) {
  setItemEnabled(1, window->hasTitlebar());
  setItemEnabled(2, window->isIconifiable());
  setItemEnabled(3, window->isMaximizable());
  setItemEnabled(8, window->isClosable());

  sendToMenu->reconfigure();

  Basemenu::reconfigure();
}


Windowmenu::SendtoWorkspacemenu::SendtoWorkspacemenu(Windowmenu *w) : Basemenu(w->screen) {
#ifdef    DEBUG
  allocate(sizeof(SendtoWorkspacemenu), "Windowmenu.cc");
#endif // DEBUG

  windowmenu = w;

  setTitleVisibility(False);
  setMovable(False);
  setInternalMenu();
  update();
}


#ifdef    DEBUG
Windowmenu::SendtoWorkspacemenu::~SendtoWorkspacemenu(void) {
  deallocate(sizeof(SendtoWorkspacemenu), "Windowmenu.cc");
}
#endif // DEBUG


void Windowmenu::SendtoWorkspacemenu::itemSelected(int button, int index) {
  if (button == 1) {
    if ((index) <= windowmenu->screen->getCount()) {
      if ((index) != windowmenu->screen->getCurrentWorkspaceID()) {
        if (windowmenu->window->isStuck()) windowmenu->window->stick();

	windowmenu->window->withdraw();
	windowmenu->screen->reassociateWindow(windowmenu->window, index, True);
	
	//	windowmenu->screen->
	//          getWorkspace(windowmenu->window->getWorkspaceNumber())->
	//          removeWindow(windowmenu->window);
	//	windowmenu->screen->getWorkspace(index)->addWindow(windowmenu->window);
	//
	//	windowmenu->window->withdraw();
      }
    }
  } else
    update();
}


void Windowmenu::SendtoWorkspacemenu::update(void) {
  int i, r = getCount();

  if (getCount() != 0)
    for (i = 0; i < r; ++i)
      remove(0);

  for (i = 0; i < windowmenu->screen->getCount(); ++i)
    insert(windowmenu->screen->getWorkspace(i)->getName());

  Basemenu::update();
}


void Windowmenu::SendtoWorkspacemenu::show(void) {
  update();

  Basemenu::show();
}

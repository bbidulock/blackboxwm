// Icon.cc for Blackbox - an X11 Window manager
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

#include "blackbox.hh"
#include "Icon.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#ifdef    DEBUG
#  include "mem.h"
#endif // DEBUG

Iconmenu::Iconmenu(BScreen *scrn) : Basemenu(scrn) {
#ifdef    DEBUG
  allocate(sizeof(Iconmenu), "Icon.cc");
#endif // DEBUG

  setInternalMenu();
  
  screen = scrn;
  blackbox = screen->getBlackbox();
  iconList = new LinkedList<BlackboxIcon>;

  setLabel("Icons");
  update();
}


Iconmenu::~Iconmenu(void) {
#ifdef    DEBUG
  deallocate(sizeof(Iconmenu), "Icon.cc");
#endif // DEBUG

  delete iconList;
}


int Iconmenu::insert(BlackboxIcon *icon) {
  icon->setIconNumber(iconList->count());
  int ret = iconList->insert(icon);
  Basemenu::insert(icon->getULabel());

  Basemenu::update();

  return ret;
}


int Iconmenu::remove(BlackboxIcon *icon) {
  iconList->remove(icon->getIconNumber());
  int ret = Basemenu::remove(icon->getIconNumber());

  int i;
  LinkedListIterator<BlackboxIcon> it(iconList);
  for (i = 0; it.current(); it++)
    it.current()->setIconNumber(i++);

  update();

  return ret;
}


void Iconmenu::itemSelected(int button, int item) {
  if (button == 1) {
    BlackboxWindow *window = iconList->find(item)->getWindow();
    screen->getWorkspace(window->getWorkspaceNumber())->raiseWindow(window);
    window->deiconify();

    if (! (screen->getWorkspacemenu()->isTorn() || isTorn()))
      hide();
  }
}


BlackboxIcon::BlackboxIcon(BlackboxWindow *win) {
#ifdef    DEBUG
  allocate(sizeof(BlackboxIcon), "Icon.cc");
#endif // DEBUG

  window = win;
  screen = window->getScreen();
  display = screen->getBlackbox()->getXDisplay();
  client = window->getClientWindow();
  icon_number = -1;

  if (! XGetIconName(display, client, &name))
    name = "Unnamed";
#ifdef    DEBUG
  else
    allocate(sizeof(char) * strlen(name), "Icon.cc");
#endif // DEBUG
  
  screen->addIcon(this);
}


BlackboxIcon::~BlackboxIcon(void) {
#ifdef    DEBUG
  deallocate(sizeof(BlackboxIcon), "Icon.cc");
#endif // DEBUG

  screen->removeIcon(this);
  
  if (name)
    if (strcmp(name, "Unnamed")) {
#ifdef    DEBUG
      deallocate(sizeof(char) * strlen(name), "Icon.cc");
#endif // DEBUG

      XFree(name);
    }
}


void BlackboxIcon::rereadLabel(void) {
  if (name)
    if (strcmp(name, "Unnamed")) {
#ifdef    DEBUG
      deallocate(sizeof(char) * strlen(name), "Icon.cc");
#endif // DEBUG

      XFree(name);
    }
  
  if (! XGetIconName(display, client, &name))
    name = "Unnamed";
#ifdef    DEBUG
  else
    allocate(sizeof(char) * strlen(name), "Icon.cc");
#endif // DEBUG
  
  screen->iconUpdate();
}

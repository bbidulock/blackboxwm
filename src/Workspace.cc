// Workspace.cc for Blackbox - an X11 Window manager
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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Windowmenu.hh"

#ifdef    DEBUG
#  include "mem.h"
#endif // DEBUG

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <string.h>
#endif // STDC_HEADERS

#define MIN(x,y) ((x < y) ? x : y)
#define MAX(x,y) ((x > y) ? x : y)


Workspace::Workspace(BScreen *scrn, int i) {
#ifdef    DEBUG
  allocate(sizeof(Workspace), "Workspace.cc");
#endif // DEBUG

  screen = scrn;

  cascade_x = cascade_y = 32;

  id = i;

  windowList = new LinkedList<BlackboxWindow>;
  clientmenu = new Clientmenu(this);

  char *tmp;  
  name = (char *) 0;
  screen->getNameOfWorkspace(id, &tmp);
  setName(tmp);
  
  if (tmp) {
    // this is the memory that is allocated in Screen.cc... see the note in
    // BScreen::getNameOfWorkspace()
    //
    //     deallocate(sizeof(char) * (strlen(tmp) + 1), "Workspace.cc");
    //

    delete [] tmp;
  }
}


Workspace::~Workspace(void) {
#ifdef    DEBUG
  deallocate(sizeof(Workspace), "Workspace.cc");
#endif // DEBUG

  delete windowList;
  delete clientmenu;

  if (name) {
#ifdef    DEBUG
    deallocate(sizeof(char) * (strlen(name) + 1), "Workspace.cc");
#endif // DEBUG

    delete [] name;
  }
}


const int Workspace::addWindow(BlackboxWindow *w, Bool place) {
  if (! w) return -1;

  if (place) placeWindow(w);

  w->setWorkspace(id);
  w->setWindowNumber(windowList->count());
  
  windowList->insert(w);
  
  clientmenu->insert(w->getTitle());
  clientmenu->update();

  screen->updateNetizenWindowAdd(w->getClientWindow(), id);

  raiseWindow(w);
  
  return w->getWindowNumber();
}


const int Workspace::removeWindow(BlackboxWindow *w) {
  if (! w) return -1;

  windowList->remove((const int) w->getWindowNumber());
  
  if (w->isFocused())
    screen->getBlackbox()->setFocusedWindow((BlackboxWindow *) 0);
  
  clientmenu->remove(w->getWindowNumber());
  clientmenu->update();

  screen->updateNetizenWindowDel(w->getClientWindow());

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (int i = 0; it.current(); it++, i++)
    it.current()->setWindowNumber(i);
  
  return windowList->count();
}


void Workspace::showAll(void) {
  BlackboxWindow *win;
  
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    win = it.current();
    if (! win->isIconic())
      win->deiconify(False);
  }
}


void Workspace::hideAll(void) {
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    BlackboxWindow *win = it.current();
    if ((! win->isIconic()) && (! win->isStuck()))
      win->withdraw();
  }
} 


void Workspace::removeAll(void) {
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    BlackboxWindow *win = it.current();
    screen->getCurrentWorkspace()->addWindow(win);
    if (! win->isIconic())
      win->iconify();
  }
}


void Workspace::raiseWindow(BlackboxWindow *w) {
  BlackboxWindow *win = (BlackboxWindow *) 0, *bottom = w;
  
  while (bottom->isTransient() && bottom->getTransientFor())
    bottom = bottom->getTransientFor();
  
  int i = 1;
  win = bottom;
  while (win->hasTransient() && win->getTransient()) {
    win = win->getTransient();
    
    i++;
  }
  
#ifdef    DEBUG
  allocate(sizeof(Window) * i, "Workspace.cc");
#endif // DEBUG

  Window *nstack = new Window[i];
  
  i = 0;
  win = bottom;
  while (win->hasTransient() && win->getTransient()) {
    *(nstack + (i++)) = win->getFrameWindow();
    screen->updateNetizenWindowRaise(win->getClientWindow());

    win = win->getTransient();
  }
  
  *(nstack + (i++)) = win->getFrameWindow();
  screen->updateNetizenWindowRaise(win->getClientWindow());

  screen->raiseWindows(nstack, i);
  
#ifdef    DEBUG
  deallocate(sizeof(Window) * i, "Workspace.cc");
#endif // DEBUG

  delete [] nstack;
}


void Workspace::lowerWindow(BlackboxWindow *w) {
  BlackboxWindow *win = (BlackboxWindow *) 0, *bottom = w;
  
  while (bottom->isTransient() && bottom->getTransientFor())
    bottom = bottom->getTransientFor();
  
  int i = 1;
  win = bottom;
  while (win->hasTransient() && win->getTransient()) {
    win = win->getTransient();
    
    i++;
  }
 
#ifdef    DEBUG
  allocate(sizeof(Window) * i, "Workspace.cc");
#endif // DEBUG

  Window *nstack = new Window[i];
 
  i = 0;
  while (win->getTransientFor()) {
    *(nstack + (i++)) = win->getFrameWindow();
    screen->updateNetizenWindowLower(win->getClientWindow());

    win = win->getTransientFor();
  }
  
  *(nstack + (i++)) = win->getFrameWindow();
  screen->updateNetizenWindowLower(win->getClientWindow());

  screen->getBlackbox()->grab();
  
  XLowerWindow(screen->getBaseDisplay()->getXDisplay(), *nstack);
  XRestackWindows(screen->getBaseDisplay()->getXDisplay(), nstack, i);
  
  screen->getBlackbox()->ungrab();

#ifdef    DEBUG  
  deallocate(sizeof(Window) * i, "Workspace.cc");
#endif // DEBUG

  delete [] nstack;
}


void Workspace::reconfigure(void) {
  clientmenu->reconfigure();

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++)
    if (it.current()->validateClient())
      it.current()->reconfigure();
}


BlackboxWindow *Workspace::getWindow(int index) {
  if ((index >= 0) && (index < windowList->count()))
    return windowList->find(index);
  else
    return 0;
}


const int Workspace::getCount(void) {
  return windowList->count();
}


void Workspace::update(void) {
  clientmenu->update();
  screen->getToolbar()->redrawWindowLabel(True);
}


Bool Workspace::isCurrent(void) {
  return (id == screen->getCurrentWorkspaceID());
}


void Workspace::setCurrent(void) {
  screen->changeWorkspaceID(id);
}


void Workspace::setName(char *new_name) {
  if (name) {
#ifdef    DEBUG
    deallocate(sizeof(char) * (strlen(name) + 1), "Workspace.cc");
#endif // DEBUG

    delete [] name;
  }
  
  if (new_name) {  
    int len = strlen(new_name) + 1;

#ifdef    DEBUG
    allocate(sizeof(char) * len, "Workspace.cc");
#endif // DEBUG

    name = new char[len];
    sprintf(name, "%s", new_name);
  } else {
    name = new char[32];
    if (name) {
      sprintf(name, "Workspace %d", id + 1);

#ifdef    DEBUG
      allocate(sizeof(char) * (strlen(name) + 1), "Workspace.cc");
#endif // DEBUG
    }
  }
  
  clientmenu->setLabel(name);
  clientmenu->update();
}


void Workspace::shutdown(void) {
  while (windowList->count()) {
    windowList->first()->restore();
    delete windowList->first();
  }
}


void Workspace::placeWindow(BlackboxWindow *win) {
  Bool placed = False;
  LinkedListIterator<BlackboxWindow> it(windowList);
  int win_w = win->getWidth() + screen->getBorderWidth2x(),
    win_h = win->getHeight() + screen->getBorderWidth2x(),
#ifdef    SLIT
    slit_w = screen->getSlit()->getWidth() + screen->getBorderWidth2x(),
    slit_h = screen->getSlit()->getHeight() + screen->getBorderWidth2x(),
#endif // SLIT
    toolbar_w = screen->getToolbar()->getWidth() + screen->getBorderWidth2x(),
    toolbar_h = screen->getToolbar()->getHeight() + screen->getBorderWidth2x(),
    place_x = 0, place_y = 0;

  register int test_x, test_y, curr_w, curr_h;

  switch (screen->getPlacementPolicy()) {
  case BScreen::RowSmartPlacement: {
    test_x = test_y = screen->getBorderWidth() +
      screen->getEdgeSnapThreshold();

    while (test_y + win_h < (signed) screen->getHeight() && ! placed) {
      test_x = screen->getBorderWidth() + screen->getEdgeSnapThreshold();

      while (test_x + win_w < (signed) screen->getWidth() && ! placed) {
        placed = True;

        it.reset();
        for (; it.current() && placed; it++) {
          if (it.current()->isIconic()) continue;

          curr_w = it.current()->getWidth() + screen->getBorderWidth2x() +
            screen->getBorderWidth2x();
          curr_h =
            ((it.current()->isShaded()) ? it.current()->getTitleHeight() :
                                          it.current()->getHeight()) +
            screen->getBorderWidth2x() + screen->getBorderWidth2x();

          if (it.current()->getXFrame() < test_x + win_w &&
              it.current()->getXFrame() + curr_w > test_x &&
              it.current()->getYFrame() < test_y + win_h &&
              it.current()->getYFrame() + curr_h > test_y) 
            placed = False;
        }

        if ((screen->getToolbar()->getX() < test_x + win_w &&
             screen->getToolbar()->getX() + toolbar_w > test_x &&
             screen->getToolbar()->getY() < test_y + win_h &&
             screen->getToolbar()->getY() + toolbar_h > test_y)
#ifdef    SLIT
             ||
            (screen->getSlit()->getX() < test_x + win_w &&
             screen->getSlit()->getX() + slit_w > test_x &&
             screen->getSlit()->getY() < test_y + win_h &&
             screen->getSlit()->getY() + slit_h > test_y)
#endif // SLIT
          )
          placed = False;

        if (placed) {
          place_x = test_x;
          place_y = test_y;

          break;
        }

        test_x += 2;
      }

      test_y += 2;
    }

    break; }

  case BScreen::ColSmartPlacement: {
    test_x = test_y = screen->getBorderWidth() +
      screen->getEdgeSnapThreshold();

    while (test_x + win_w < (signed) screen->getWidth() && ! placed) {
      test_y = screen->getBorderWidth() + screen->getEdgeSnapThreshold();

      while (test_y + win_h < (signed) screen->getHeight() && ! placed) {
        placed = True;

        it.reset();
        for (; it.current() && placed; it++) {
          if (it.current()->isIconic()) continue;

          curr_w = it.current()->getWidth() + screen->getBorderWidth2x() +
            screen->getBorderWidth2x();
          curr_h =
            ((it.current()->isShaded()) ? it.current()->getTitleHeight() :
                                          it.current()->getHeight()) +
            screen->getBorderWidth2x() + screen->getBorderWidth2x();

          if (it.current()->getXFrame() < test_x + win_w &&
              it.current()->getXFrame() + curr_w > test_x &&
              it.current()->getYFrame() < test_y + win_h &&
              it.current()->getYFrame() + curr_h > test_y) 
            placed = False;
        }

        if ((screen->getToolbar()->getX() < test_x + win_w &&
             screen->getToolbar()->getX() + toolbar_w > test_x &&
             screen->getToolbar()->getY() < test_y + win_h &&
             screen->getToolbar()->getY() + toolbar_h > test_y)
#ifdef    SLIT
             ||
            (screen->getSlit()->getX() < test_x + win_w &&
             screen->getSlit()->getX() + slit_w > test_x &&
             screen->getSlit()->getY() < test_y + win_h &&
             screen->getSlit()->getY() + slit_h > test_y)
#endif // SLIT
         )
         placed = False;

       if (placed) {
         place_x = test_x;
         place_y = test_y;

         break;
       }

       test_y += 2;
     }

     test_x += 2;
   }

   break; }
  }
    
  if (! placed) {
    if (((unsigned) cascade_x > (screen->getWidth() / 2)) ||
	((unsigned) cascade_y > (screen->getHeight() / 2)))
      cascade_x = cascade_y = 32;
    
    place_x = cascade_x;
    place_y = cascade_y;
    
    cascade_x += win->getTitleHeight();
    cascade_y += win->getTitleHeight();
  }
  
  if (place_x + win_w > (signed) screen->getWidth())
    place_x = (screen->getWidth() - win_w) / 2;
  if (place_y + win_h > (signed) screen->getHeight())
    place_y = (screen->getHeight() - win_h) / 2;
  
  win->configure(place_x, place_y, win->getWidth(), win->getHeight());
}

// Configmenu.cc for Blackbox - An X11 Window Manager
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
#  define _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
# include "../config.h"
#endif // HAVE_CONFIG_H

#include "Configmenu.hh"
#include "Toolbar.hh"
#include "Window.hh"

#ifdef    DEBUG
#  include "mem.h"
#endif // DEBUG


Configmenu::Configmenu(BScreen *scr) : Basemenu(scr) {
#ifdef    DEBUG
  allocate(sizeof(Configmenu), "Configmenu.cc");
#endif // DEBUG

  screen = scr;
  blackbox = screen->getBlackbox();
  setLabel("Config options");
  setInternalMenu();

  focusmenu = new Focusmenu(this);
  placementmenu = new Placementmenu(this);

  insert("Focus Model", focusmenu);
  insert("Window Placement", placementmenu);
  insert("Image Dithering", 1);
  insert("Opaque Window Moving", 2);
  insert("Full Maximization", 3);
  insert("Focus New Windows", 4);
  
  update();

  setItemSelected(2, screen->getImageControl()->doDither());
  setItemSelected(3, screen->doOpaqueMove());
  setItemSelected(4, screen->doFullMax());
  setItemSelected(5, screen->doFocusNew());
}


Configmenu::~Configmenu(void) {
#ifdef    DEBUG
  deallocate(sizeof(Configmenu), "Configmenu.cc");
#endif // DEBUG

  delete focusmenu;
  delete placementmenu;
}


void Configmenu::itemSelected(int button, int index) {
  if (button == 1) {
    BasemenuItem *item = find(index);

    if (item->function())
      switch(item->function()) {
      case 1: { // dither
        Bool change = ((screen->getImageControl()->doDither()) ?  False : True);
	screen->getImageControl()->setDither(change);
	  
	setItemSelected(index, screen->getImageControl()->doDither());

        break; }

      case 2: { // opaque move
        Bool change = (! screen->doOpaqueMove());
	screen->saveOpaqueMove(change);
	  
	setItemSelected(index, screen->doOpaqueMove());

        break; }

      case 3: { // full maximization
        Bool change = (! screen->doFullMax());
        screen->saveFullMax(change);

        setItemSelected(index, screen->doFullMax());

        break; }
      case 4: { // focus new windows
        Bool change = (! screen->doFocusNew());
        screen->saveFocusNew(change);

        setItemSelected(index, screen->doFocusNew());
        break; }
      }
    }
}


void Configmenu::reconfigure(void) {
  focusmenu->reconfigure();
  placementmenu->reconfigure();

  Basemenu::reconfigure();
}


Configmenu::Focusmenu::Focusmenu(Configmenu *cm) : Basemenu(cm->screen) {
#ifdef    DEBUG
  allocate(sizeof(Focusmenu), "Configmenu.cc");
#endif // DEBUG

  configmenu = cm;

  setLabel("Focus Model");
  setInternalMenu();

  insert("Click To Focus", 1);
  insert("Sloppy Focus", 2);
  insert("Auto Raise", 3);

  update();

  setItemSelected(0, (! configmenu->screen->isSloppyFocus()));
  setItemSelected(1, configmenu->screen->isSloppyFocus());
  setItemEnabled(2, configmenu->screen->isSloppyFocus());
  setItemSelected(2, configmenu->screen->doAutoRaise());
}


#ifdef    DEBUG
Configmenu::Focusmenu::~Focusmenu(void) {
  deallocate(sizeof(Focusmenu), "Configmenu.cc");
}
#endif // DEBUG


void Configmenu::Focusmenu::itemSelected(int button, int index) {
  if (button == 1) {
    BasemenuItem *item = find(index);

    if (item->function()) {
      switch (item->function()) {
      case 1: // click to focus
	configmenu->screen->saveSloppyFocus(False);
	configmenu->screen->saveAutoRaise(False);

	if (! configmenu->screen->getBlackbox()->getFocusedWindow())
	  XSetInputFocus(configmenu->screen->getBlackbox()->getXDisplay(),
			 configmenu->screen->getToolbar()->getWindowID(),
			 RevertToParent, CurrentTime);
	else
	  XSetInputFocus(configmenu->screen->getBlackbox()->getXDisplay(),
			 configmenu->screen->getBlackbox()->
			   getFocusedWindow()->getClientWindow(),
			 RevertToParent, CurrentTime);

	configmenu->screen->reconfigure();

	break;

      case 2: // sloppy focus
	configmenu->screen->saveSloppyFocus(True);

	configmenu->screen->reconfigure();
	
	break;

      case 3: // auto raise with sloppy focus
	Bool change = ((configmenu->screen->doAutoRaise()) ? False : True);
	configmenu->screen->saveAutoRaise(change);

	break;
      }
      
      setItemSelected(0, (! configmenu->screen->isSloppyFocus()));
      setItemSelected(1, configmenu->screen->isSloppyFocus());
      setItemEnabled(2, configmenu->screen->isSloppyFocus());
      setItemSelected(2, configmenu->screen->doAutoRaise());
    }
  }
}


Configmenu::Placementmenu::Placementmenu(Configmenu *cm) : Basemenu(cm->screen) {
#ifdef    DEBUG
  allocate(sizeof(Placementmenu), "Configmenu.cc");
#endif // DEBUG

  configmenu = cm;

  setLabel("Window Placement");
  setInternalMenu();

  insert("Smart Placement (Rows)", BScreen::RowSmartPlacement);
  insert("Smart Placement (Columns)", BScreen::ColSmartPlacement);
  insert("Cascade Placement", BScreen::CascadePlacement);

  update();

  switch (configmenu->screen->getPlacementPolicy()) {
  case BScreen::RowSmartPlacement:
    setItemSelected(0, True);
    break;

  case BScreen::ColSmartPlacement:
    setItemSelected(1, True);
    break;

  case BScreen::CascadePlacement:
    setItemSelected(2, True);
    break;
  }
}


#ifdef    DEBUG
Configmenu::Placementmenu::~Placementmenu(void) {
  deallocate(sizeof(Placementmenu), "Configmenu.cc");
}
#endif // DEBUG


void Configmenu::Placementmenu::itemSelected(int button, int index) {
  if (button == 1) {
    BasemenuItem *item = find(index);

    if (item->function()) {
      switch (item->function()) {
      case BScreen::RowSmartPlacement:
	configmenu->screen->savePlacementPolicy(item->function());

	setItemSelected(0, True);
	setItemSelected(1, False);
        setItemSelected(2, False);
	
	break;

      case BScreen::ColSmartPlacement:
        configmenu->screen->savePlacementPolicy(item->function());

        setItemSelected(0, False);
        setItemSelected(1, True);
        setItemSelected(2, False);

        break;
	
      case  BScreen::CascadePlacement:
	configmenu->screen->savePlacementPolicy(item->function());

	setItemSelected(0, False);
        setItemSelected(1, False);
	setItemSelected(2, True);

	break;
      }
    }
  }
}

// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Configmenu.cc for Blackbox - An X11 Window Manager
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
# include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "Configmenu.hh"

#include "Image.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Screen.hh"
#include "blackbox.hh"
#include "i18n.hh"


Configmenu::Configmenu(BScreen *scr)
    : Basemenu(scr->screen())
{
  screen = scr;
  blackbox = Blackbox::instance();
  setTitle(i18n->getMessage(ConfigmenuSet, ConfigmenuConfigOptions,
                            "Config options"));
  showTitle();

  focusmenu = new Focusmenu(this);
  placementmenu = new Placementmenu(this);

  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuFocusModel,
                          "Focus Model"), focusmenu);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuWindowPlacement,
                          "Window Placement"), placementmenu);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuImageDithering,
                          "Image Dithering"), 1);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuOpaqueMove,
                          "Opaque Window Moving"), 2);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuFullMax,
                          "Full Maximization"), 3);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuFocusNew,
                          "Focus New Windows"), 4);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuFocusLast,
                          "Focus Last Window on Workspace"), 5);

  setItemChecked(2, screen->getImageControl()->doDither());
  setItemChecked(3, screen->doOpaqueMove());
  setItemChecked(4, screen->doFullMax());
  setItemChecked(5, screen->doFocusNew());
  setItemChecked(6, screen->doFocusLast());
}

Configmenu::~Configmenu(void) {
  delete focusmenu;
  delete placementmenu;
}

void Configmenu::itemClicked(const Point &, const Item &item, int button)
{
  if (button != 1)
    return;

  if (! item.function())
    return;

  switch(item.function()) {
  case 1: { // dither
    screen->getImageControl()->
      setDither((! screen->getImageControl()->doDither()));

    setItemChecked(item.index(), screen->getImageControl()->doDither());

    break;
  }

  case 2: { // opaque move
    screen->saveOpaqueMove((! screen->doOpaqueMove()));

    setItemChecked(item.index(), screen->doOpaqueMove());

    break;
  }

  case 3: { // full maximization
    screen->saveFullMax((! screen->doFullMax()));

    setItemChecked(item.index(), screen->doFullMax());

    break;
  }
  case 4: { // focus new windows
    screen->saveFocusNew((! screen->doFocusNew()));

    setItemChecked(item.index(), screen->doFocusNew());
    break;
  }

  case 5: { // focus last window on workspace
    screen->saveFocusLast((! screen->doFocusLast()));
    setItemChecked(item.index(), screen->doFocusLast());
    break;
  }
  } // switch
}

void Configmenu::reconfigure(void) {
  focusmenu->reconfigure();
  placementmenu->reconfigure();

  Basemenu::reconfigure();
}

Configmenu::Focusmenu::Focusmenu(Configmenu *cm)
  : Basemenu(cm->screen->screen())
{
  configmenu = cm;

  setTitle(i18n->getMessage(ConfigmenuSet, ConfigmenuFocusModel,
			    "Focus Model"));
  showTitle();

  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuClickToFocus,
			  "Click To Focus"), 1);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuSloppyFocus,
			  "Sloppy Focus"), 2);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuAutoRaise,
			  "Auto Raise"), 3);

  setItemChecked(0, (! configmenu->screen->isSloppyFocus()));
  setItemChecked(1, configmenu->screen->isSloppyFocus());
  setItemEnabled(2, configmenu->screen->isSloppyFocus());
  setItemChecked(2, configmenu->screen->doAutoRaise());
}

void Configmenu::Focusmenu::itemClicked(const Point &, const Item &item, int button)
{
  if (button != 1)
    return;

  if (!item.function())
    return;

  switch (item.function()) {
  case 1: // click to focus
    configmenu->screen->saveSloppyFocus(False);
    configmenu->screen->saveAutoRaise(False);

    if (! Blackbox::instance()->getFocusedWindow())
      XSetInputFocus( *BaseDisplay::instance(),
                      configmenu->screen->getToolbar()->getWindowID(),
                      RevertToParent, CurrentTime);
    else
      XSetInputFocus( *BaseDisplay::instance(),
                      Blackbox::instance()->getFocusedWindow()->getClientWindow(),
                      RevertToParent, CurrentTime);
    hideAll();
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

  setItemChecked(0, (! configmenu->screen->isSloppyFocus()));
  setItemChecked(1, configmenu->screen->isSloppyFocus());
  setItemEnabled(2, configmenu->screen->isSloppyFocus());
  setItemChecked(2, configmenu->screen->doAutoRaise());
}

Configmenu::Placementmenu::Placementmenu(Configmenu *cm) :
 Basemenu(cm->screen->screen())
{
  configmenu = cm;

  setTitle(i18n->getMessage(ConfigmenuSet, ConfigmenuWindowPlacement,
			    "Window Placement"));
  showTitle();

  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuSmartRows,
			  "Smart Placement (Rows)"),
	 BScreen::RowSmartPlacement);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuSmartCols,
			  "Smart Placement (Columns)"),
	 BScreen::ColSmartPlacement);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuCascade,
			  "Cascade Placement"), BScreen::CascadePlacement);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuLeftRight,
			  "Left to Right"), BScreen::LeftRight);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuRightLeft,
			  "Right to Left"), BScreen::RightLeft);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuTopBottom,
			  "Top to Bottom"), BScreen::TopBottom);
  insert(i18n->getMessage(ConfigmenuSet, ConfigmenuBottomTop,
			  "Bottom to Top"), BScreen::BottomTop);

  switch (configmenu->screen->getPlacementPolicy()) {
  case BScreen::RowSmartPlacement:
    setItemChecked(0, True);
    break;

  case BScreen::ColSmartPlacement:
    setItemChecked(1, True);
    break;

  case BScreen::CascadePlacement:
    setItemChecked(2, True);
    break;
  }

  Bool rl = (configmenu->screen->getRowPlacementDirection() ==
	     BScreen::LeftRight),
       tb = (configmenu->screen->getColPlacementDirection() ==
	     BScreen::TopBottom);

  setItemChecked(3, rl);
  setItemChecked(4, ! rl);

  setItemChecked(5, tb);
  setItemChecked(6, ! tb);
}

void Configmenu::Placementmenu::itemClicked(const Point &, const Item &item, int button)
{
  if (button != 1)
    return;

  if (!item.function())
    return;

  switch (item.function()) {
  case BScreen::RowSmartPlacement:
    configmenu->screen->savePlacementPolicy(item.function());

    setItemChecked(0, True);
    setItemChecked(1, False);
    setItemChecked(2, False);

    break;

  case BScreen::ColSmartPlacement:
    configmenu->screen->savePlacementPolicy(item.function());

    setItemChecked(0, False);
    setItemChecked(1, True);
    setItemChecked(2, False);

    break;

  case BScreen::CascadePlacement:
    configmenu->screen->savePlacementPolicy(item.function());

    setItemChecked(0, False);
    setItemChecked(1, False);
    setItemChecked(2, True);

    break;

  case BScreen::LeftRight:
    configmenu->screen->saveRowPlacementDirection(BScreen::LeftRight);

    setItemChecked(3, True);
    setItemChecked(4, False);

    break;

  case BScreen::RightLeft:
    configmenu->screen->saveRowPlacementDirection(BScreen::RightLeft);

    setItemChecked(3, False);
    setItemChecked(4, True);

    break;

  case BScreen::TopBottom:
    configmenu->screen->saveColPlacementDirection(BScreen::TopBottom);

    setItemChecked(5, True);
    setItemChecked(6, False);

    break;

  case BScreen::BottomTop:
    configmenu->screen->saveColPlacementDirection(BScreen::BottomTop);

    setItemChecked(5, False);
    setItemChecked(6, True);

    break;
  }
}

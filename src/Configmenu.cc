// -*- mode: C++; indent-tabs-mode: nil; -*-
// Configmenu.cc for Blackbox - An X11 Window Manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
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

#ifdef    HAVE_CONFIG_H
# include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "Configmenu.hh"
#include "Image.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Screen.hh"

Configmenu::Configmenu(BScreen *scr) : Basemenu(scr) {
  screen = scr;
  blackbox = screen->getBlackbox();
  setLabel(i18n(ConfigmenuSet, ConfigmenuConfigOptions, "Config options"));
  setInternalMenu();

  focusmenu = new Focusmenu(this);
  placementmenu = new Placementmenu(this);

  insert(i18n(ConfigmenuSet, ConfigmenuFocusModel,
              "Focus Model"), focusmenu);
  insert(i18n(ConfigmenuSet, ConfigmenuWindowPlacement,
              "Window Placement"), placementmenu);
  insert(i18n(ConfigmenuSet, ConfigmenuImageDithering,
              "Image Dithering"), 1);
  insert(i18n(ConfigmenuSet, ConfigmenuOpaqueMove,
              "Opaque Window Moving"), 2);
  insert(i18n(ConfigmenuSet, ConfigmenuFullMax,
              "Full Maximization"), 3);
  insert(i18n(ConfigmenuSet, ConfigmenuFocusNew,
              "Focus New Windows"), 4);
  insert(i18n(ConfigmenuSet, ConfigmenuFocusLast,
              "Focus Last Window on Workspace"), 5);
  update();

  setItemSelected(2, screen->getImageControl()->doDither());
  setItemSelected(3, screen->doOpaqueMove());
  setItemSelected(4, screen->doFullMax());
  setItemSelected(5, screen->doFocusNew());
  setItemSelected(6, screen->doFocusLast());
}

Configmenu::~Configmenu(void) {
  delete focusmenu;
  delete placementmenu;
}

void Configmenu::itemSelected(int button, int index) {
  if (button != 1)
    return;

  BasemenuItem *item = find(index);

  if (!item->function())
    return;

  switch(item->function()) {
  case 1: { // dither
    screen->getImageControl()->
      setDither((! screen->getImageControl()->doDither()));

    setItemSelected(index, screen->getImageControl()->doDither());

    break;
  }

  case 2: { // opaque move
    screen->saveOpaqueMove((! screen->doOpaqueMove()));

    setItemSelected(index, screen->doOpaqueMove());

    break;
  }

  case 3: { // full maximization
    screen->saveFullMax((! screen->doFullMax()));

    setItemSelected(index, screen->doFullMax());

    break;
  }
  case 4: { // focus new windows
    screen->saveFocusNew((! screen->doFocusNew()));

    setItemSelected(index, screen->doFocusNew());
    break;
  }

  case 5: { // focus last window on workspace
    screen->saveFocusLast((! screen->doFocusLast()));
    setItemSelected(index, screen->doFocusLast());
    break;
  }
  } // switch
}

void Configmenu::reconfigure(void) {
  focusmenu->reconfigure();
  placementmenu->reconfigure();

  Basemenu::reconfigure();
}

Configmenu::Focusmenu::Focusmenu(Configmenu *cm) : Basemenu(cm->screen) {
  configmenu = cm;

  setLabel(i18n(ConfigmenuSet, ConfigmenuFocusModel,
                "Focus Model"));
  setInternalMenu();

  insert(i18n(ConfigmenuSet, ConfigmenuClickToFocus,
              "Click To Focus"), 1);
  insert(i18n(ConfigmenuSet, ConfigmenuSloppyFocus,
              "Sloppy Focus"), 2);
  insert(i18n(ConfigmenuSet, ConfigmenuAutoRaise,
              "Auto Raise"), 3);
  update();

  setItemSelected(0, (! configmenu->screen->isSloppyFocus()));
  setItemSelected(1, configmenu->screen->isSloppyFocus());
  setItemEnabled(2, configmenu->screen->isSloppyFocus());
  setItemSelected(2, configmenu->screen->doAutoRaise());
}

void Configmenu::Focusmenu::itemSelected(int button, int index) {
  if (button != 1)
    return;

  BasemenuItem *item = find(index);

  if (!item->function())
    return;

  switch (item->function()) {
  case 1: // click to focus
    configmenu->screen->toggleFocusModel(BScreen::ClickToFocus);
    break;

  case 2: // sloppy focus
    configmenu->screen->toggleFocusModel(BScreen::SloppyFocus);

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

Configmenu::Placementmenu::Placementmenu(Configmenu *cm) :
  Basemenu(cm->screen) {
  configmenu = cm;

  setLabel(i18n(ConfigmenuSet, ConfigmenuWindowPlacement,
                "Window Placement"));
  setInternalMenu();

  insert(i18n(ConfigmenuSet, ConfigmenuSmartRows,
              "Smart Placement (Rows)"),
         BScreen::RowSmartPlacement);
  insert(i18n(ConfigmenuSet, ConfigmenuSmartCols,
              "Smart Placement (Columns)"),
         BScreen::ColSmartPlacement);
  insert(i18n(ConfigmenuSet, ConfigmenuCascade,
              "Cascade Placement"), BScreen::CascadePlacement);
  insert(i18n(ConfigmenuSet, ConfigmenuLeftRight,
              "Left to Right"), BScreen::LeftRight);
  insert(i18n(ConfigmenuSet, ConfigmenuRightLeft,
              "Right to Left"), BScreen::RightLeft);
  insert(i18n(ConfigmenuSet, ConfigmenuTopBottom,
              "Top to Bottom"), BScreen::TopBottom);
  insert(i18n(ConfigmenuSet, ConfigmenuBottomTop,
              "Bottom to Top"), BScreen::BottomTop);
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

  Bool rl = (configmenu->screen->getRowPlacementDirection() ==
             BScreen::LeftRight),
    tb = (configmenu->screen->getColPlacementDirection() ==
          BScreen::TopBottom);

  setItemSelected(3, rl);
  setItemSelected(4, ! rl);

  setItemSelected(5, tb);
  setItemSelected(6, ! tb);
}

void Configmenu::Placementmenu::itemSelected(int button, int index) {
  if (button != 1)
    return;

  BasemenuItem *item = find(index);

  if (!item->function())
    return;

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

  case BScreen::CascadePlacement:
    configmenu->screen->savePlacementPolicy(item->function());

    setItemSelected(0, False);
    setItemSelected(1, False);
    setItemSelected(2, True);

    break;

  case BScreen::LeftRight:
    configmenu->screen->saveRowPlacementDirection(BScreen::LeftRight);

    setItemSelected(3, True);
    setItemSelected(4, False);

    break;

  case BScreen::RightLeft:
    configmenu->screen->saveRowPlacementDirection(BScreen::RightLeft);

    setItemSelected(3, False);
    setItemSelected(4, True);

    break;

  case BScreen::TopBottom:
    configmenu->screen->saveColPlacementDirection(BScreen::TopBottom);

    setItemSelected(5, True);
    setItemSelected(6, False);

    break;

  case BScreen::BottomTop:
    configmenu->screen->saveColPlacementDirection(BScreen::BottomTop);

    setItemSelected(5, False);
    setItemSelected(6, True);

    break;
  }
}

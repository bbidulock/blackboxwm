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

#include "Configmenu.hh"

#include "Image.hh"
#include "Screen.hh"
#include "i18n.hh"


class ConfigFocusmenu : public bt::Menu {
public:
  ConfigFocusmenu(bt::Application &app, unsigned int screen, BScreen *bscreen);

  void refresh(void);

protected:
  void itemClicked(unsigned int id, unsigned int button);

private:
  BScreen *_bscreen;
};


class ConfigPlacementmenu : public bt::Menu {
public:
  ConfigPlacementmenu(bt::Application &app, unsigned int screen,
                      BScreen *bscreen);

  void refresh(void);

protected:
  void itemClicked(unsigned int id, unsigned int button);

private:
  BScreen *_bscreen;
};


Configmenu::Configmenu(bt::Application &app, unsigned int screen,
                       BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen) {
  setTitle(bt::i18n(ConfigmenuSet, ConfigmenuConfigOptions, "Config options"));
  showTitle();

  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuFocusModel,
                      "Focus Model"),
             new ConfigFocusmenu(app, screen, bscreen),
             ConfigmenuFocusModel);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuWindowPlacement,
                      "Window Placement"),
             new ConfigPlacementmenu(app, screen, bscreen),
             ConfigmenuWindowPlacement);

  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuImageDithering,
                      "Image Dithering"),
             ConfigmenuImageDithering);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuOpaqueMove,
                      "Opaque Window Moving"),
             ConfigmenuOpaqueMove);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuFullMax,
                      "Full Maximization"),
             ConfigmenuFullMax);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuFocusNew,
                      "Focus New Windows"),
             ConfigmenuFocusNew);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuFocusLast,
                      "Focus Last Window on Workspace"),
             ConfigmenuFocusLast);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuDisableBindings,
                      "Disable Bindings with Scroll Lock"),
             ConfigmenuDisableBindings);
}


void Configmenu::refresh(void) {
  setItemChecked(ConfigmenuImageDithering,
                 _bscreen->getImageControl()->doDither());
  setItemChecked(ConfigmenuOpaqueMove,
                 _bscreen->doOpaqueMove());
  setItemChecked(ConfigmenuFullMax,
                 _bscreen->doFullMax());
  setItemChecked(ConfigmenuFocusNew,
                 _bscreen->doFocusNew());
  setItemChecked(ConfigmenuFocusLast,
                 _bscreen->doFocusLast());
  setItemChecked(ConfigmenuDisableBindings,
                 _bscreen->allowScrollLock());
}


void Configmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  switch (id) {
  case ConfigmenuImageDithering: // dither
    _bscreen->getImageControl()->
      setDither(! _bscreen->getImageControl()->doDither());
    break;

  case ConfigmenuOpaqueMove: // opaque move
    _bscreen->saveOpaqueMove(! _bscreen->doOpaqueMove());
    break;

  case ConfigmenuFullMax: // full maximization
    _bscreen->saveFullMax(! _bscreen->doFullMax());
    break;

  case ConfigmenuFocusNew: // focus new windows
    _bscreen->saveFocusNew(! _bscreen->doFocusNew());
    break;

  case ConfigmenuFocusLast: // focus last window on workspace
    _bscreen->saveFocusLast(! _bscreen->doFocusLast());
    break;

  case ConfigmenuDisableBindings: // disable keybindings with Scroll Lock
    _bscreen->saveAllowScrollLock(! _bscreen->allowScrollLock());
    _bscreen->reconfigure();
    break;
  } // switch
}

static const unsigned int AutoRaiseID = BScreen::SloppyFocus + 10;
static const unsigned int ClickRaiseID = BScreen::SloppyFocus + 11;

ConfigFocusmenu::ConfigFocusmenu(bt::Application &app, unsigned int screen,
                                 BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen) {
  setTitle(bt::i18n(ConfigmenuSet, ConfigmenuFocusModel, "Focus Model"));
  showTitle();

  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuClickToFocus, "Click To Focus"),
             BScreen::ClickToFocus);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuSloppyFocus, "Sloppy Focus"),
             BScreen::SloppyFocus);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuAutoRaise, "Auto Raise"),
             AutoRaiseID);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuClickRaise, "Click Raise"),
             ClickRaiseID);
}


void ConfigFocusmenu::refresh(void) {
  setItemChecked(BScreen::ClickToFocus,
                 !_bscreen->isSloppyFocus());
  setItemChecked(BScreen::SloppyFocus,
                 _bscreen->isSloppyFocus());
  setItemEnabled(AutoRaiseID,
                 _bscreen->isSloppyFocus());
  setItemChecked(AutoRaiseID,
                 _bscreen->doAutoRaise());
  setItemEnabled(ClickRaiseID,
                 _bscreen->isSloppyFocus());
  setItemChecked(ClickRaiseID,
                 _bscreen->doClickRaise());
}


void ConfigFocusmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  switch (id) {
  case BScreen::ClickToFocus:
  case BScreen::SloppyFocus:
    _bscreen->toggleFocusModel((BScreen::FocusModel) id);
    break;

  case AutoRaiseID: // auto raise with sloppy focus
    _bscreen->saveAutoRaise(! _bscreen->doAutoRaise());
    break;

  case ClickRaiseID: // click raise with sloppy focus
    _bscreen->saveClickRaise(! _bscreen->doClickRaise());
    // make sure the appropriate mouse buttons are grabbed on the windows
    _bscreen->toggleFocusModel(BScreen::SloppyFocus);
    break;
  } // switch
}


ConfigPlacementmenu::ConfigPlacementmenu(bt::Application &app,
                                         unsigned int screen,
                                         BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen) {
  setTitle(bt::i18n(ConfigmenuSet, ConfigmenuWindowPlacement,
                    "Window Placement"));
  showTitle();

  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuSmartRows,
                      "Smart Placement (Rows)"),
             BScreen::RowSmartPlacement);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuSmartCols,
                      "Smart Placement (Columns)"),
             BScreen::ColSmartPlacement);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuCascade,
                      "Cascade Placement"),
             BScreen::CascadePlacement);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuLeftRight,
                      "Left to Right"),
             BScreen::LeftRight);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuRightLeft,
                      "Right to Left"),
             BScreen::RightLeft);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuTopBottom,
                      "Top to Bottom"),
             BScreen::TopBottom);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuBottomTop,
                      "Bottom to Top"),
             BScreen::BottomTop);
}


void ConfigPlacementmenu::refresh(void) {
  bool rowsmart = _bscreen->getPlacementPolicy() == BScreen::RowSmartPlacement,
       colsmart = _bscreen->getPlacementPolicy() == BScreen::ColSmartPlacement,
        cascade = _bscreen->getPlacementPolicy() == BScreen::CascadePlacement,
             rl = _bscreen->getRowPlacementDirection() == BScreen::LeftRight,
             tb = _bscreen->getColPlacementDirection() == BScreen::TopBottom;

  setItemChecked(BScreen::RowSmartPlacement, rowsmart);
  setItemChecked(BScreen::ColSmartPlacement, colsmart);
  setItemChecked(BScreen::CascadePlacement, cascade);

  setItemEnabled(BScreen::LeftRight, ! cascade);
  setItemChecked(BScreen::LeftRight, cascade || rl);

  setItemEnabled(BScreen::RightLeft, ! cascade);
  setItemChecked(BScreen::RightLeft, ! cascade && ! rl);

  setItemEnabled(BScreen::TopBottom, ! cascade);
  setItemChecked(BScreen::TopBottom, cascade || tb);

  setItemEnabled(BScreen::BottomTop, ! cascade);
  setItemChecked(BScreen::BottomTop, ! cascade && ! tb);
}


void ConfigPlacementmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  switch (id) {
  case BScreen::RowSmartPlacement:
  case BScreen::ColSmartPlacement:
  case BScreen::CascadePlacement:
    _bscreen->savePlacementPolicy(id);
    break;

  case BScreen::LeftRight:
  case BScreen::RightLeft:
    _bscreen->saveRowPlacementDirection(id);
    break;

  case BScreen::TopBottom:
  case BScreen::BottomTop:
    _bscreen->saveColPlacementDirection(id);
    break;
  } // switch
}

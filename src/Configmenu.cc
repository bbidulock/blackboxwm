// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Configmenu.cc for Blackbox - An X11 Window Manager
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
  void itemClicked(unsigned int id, unsigned int);

private:
  BScreen *_bscreen;
};


class ConfigPlacementmenu : public bt::Menu {
public:
  ConfigPlacementmenu(bt::Application &app, unsigned int screen,
                      BScreen *bscreen);

  void refresh(void);

protected:
  void itemClicked(unsigned int id, unsigned int);

private:
  BScreen *_bscreen;
};


class ConfigDithermenu : public bt::Menu {
public:
  ConfigDithermenu(bt::Application &app, unsigned int screen);

  void refresh(void);

protected:
  void itemClicked(unsigned int id, unsigned int);
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
             new ConfigDithermenu(app, screen),
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
  ScreenResource& res = _bscreen->resource();
  setItemChecked(ConfigmenuOpaqueMove, res.doOpaqueMove());
  setItemChecked(ConfigmenuFullMax, res.doFullMax());
  setItemChecked(ConfigmenuFocusNew, res.doFocusNew());
  setItemChecked(ConfigmenuFocusLast, res.doFocusLast());
  setItemChecked(ConfigmenuDisableBindings, res.allowScrollLock());
}


void Configmenu::itemClicked(unsigned int id, unsigned int) {
  ScreenResource& res = _bscreen->resource();
  switch (id) {
  case ConfigmenuOpaqueMove: // opaque move
    res.saveOpaqueMove(! res.doOpaqueMove());
    break;

  case ConfigmenuFullMax: // full maximization
    res.saveFullMax(! res.doFullMax());
    break;

  case ConfigmenuFocusNew: // focus new windows
    res.saveFocusNew(! res.doFocusNew());
    break;

  case ConfigmenuFocusLast: // focus last window on workspace
    res.saveFocusLast(! res.doFocusLast());
    break;

  case ConfigmenuDisableBindings: // disable keybindings with Scroll Lock
    res.saveAllowScrollLock(! res.allowScrollLock());
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
  ScreenResource& res = _bscreen->resource();
  setItemChecked(BScreen::ClickToFocus, !res.isSloppyFocus());
  setItemChecked(BScreen::SloppyFocus, res.isSloppyFocus());
  setItemEnabled(AutoRaiseID, res.isSloppyFocus());
  setItemChecked(AutoRaiseID, res.doAutoRaise());
  setItemEnabled(ClickRaiseID, res.isSloppyFocus());
  setItemChecked(ClickRaiseID, res.doClickRaise());
}


void ConfigFocusmenu::itemClicked(unsigned int id, unsigned int) {
  ScreenResource& res = _bscreen->resource();
  switch (id) {
  case BScreen::ClickToFocus:
  case BScreen::SloppyFocus:
    _bscreen->toggleFocusModel((BScreen::FocusModel) id);
    break;

  case AutoRaiseID: // auto raise with sloppy focus
    res.saveAutoRaise(! res.doAutoRaise());
    break;

  case ClickRaiseID: // click raise with sloppy focus
    res.saveClickRaise(! res.doClickRaise());
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
             RowSmartPlacement);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuSmartCols,
                      "Smart Placement (Columns)"),
             ColSmartPlacement);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuCascade,
                      "Cascade Placement"),
             CascadePlacement);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuLeftRight,
                      "Left to Right"),
             LeftRight);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuRightLeft,
                      "Right to Left"),
             RightLeft);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuTopBottom,
                      "Top to Bottom"),
             TopBottom);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuBottomTop,
                      "Bottom to Top"),
             BottomTop);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuIgnoreShaded,
                      "Ignore Shaded Windows"),
             ConfigmenuIgnoreShaded);
}


void ConfigPlacementmenu::refresh(void) {
  ScreenResource& res = _bscreen->resource();
  bool rowsmart = res.placementPolicy() == RowSmartPlacement,
       colsmart = res.placementPolicy() == ColSmartPlacement,
        cascade = res.placementPolicy() == CascadePlacement,
             rl = res.rowPlacementDirection() == LeftRight,
             tb = res.colPlacementDirection() == TopBottom;

  setItemChecked(RowSmartPlacement, rowsmart);
  setItemChecked(ColSmartPlacement, colsmart);
  setItemChecked(CascadePlacement, cascade);

  setItemEnabled(LeftRight, ! cascade);
  setItemChecked(LeftRight, cascade || rl);

  setItemEnabled(RightLeft, ! cascade);
  setItemChecked(RightLeft, ! cascade && ! rl);

  setItemEnabled(TopBottom, ! cascade);
  setItemChecked(TopBottom, cascade || tb);

  setItemEnabled(BottomTop, ! cascade);
  setItemChecked(BottomTop, ! cascade && ! tb);

  setItemChecked(ConfigmenuIgnoreShaded, res.placementIgnoresShaded());
}


void ConfigPlacementmenu::itemClicked(unsigned int id, unsigned int) {
  ScreenResource& res = _bscreen->resource();
  switch (id) {
  case RowSmartPlacement:
  case ColSmartPlacement:
  case CascadePlacement: {
    PlacementPolicy p = static_cast<PlacementPolicy>(id);
    res.savePlacementPolicy(p);
  }
    break;

  case LeftRight:
  case RightLeft: {
    PlacementDirection d = static_cast<PlacementDirection>(id);
    res.saveRowPlacementDirection(d);
  }
    break;

  case TopBottom:
  case BottomTop: {
    PlacementDirection d = static_cast<PlacementDirection>(id);
    res.saveColPlacementDirection(d);
  }
    break;

  case ConfigmenuIgnoreShaded:
    res.savePlacementIgnoresShaded(! res.placementIgnoresShaded());
  } // switch
}


ConfigDithermenu::ConfigDithermenu(bt::Application &app,
                                   unsigned int screen)
  : bt::Menu(app, screen) {
  setTitle(bt::i18n(ConfigmenuSet, ConfigmenuImageDithering,
                    "Image Dithering"));
  showTitle();

  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuNoDithering,
                      "Do not dither images"),
             bt::NoDither);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuOrderedDithering,
                      "Use fast dither"),
             bt::OrderedDither);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuFloydSteinbergDithering,
                      "Use high-quality dither"),
             bt::FloydSteinbergDither);
}


void ConfigDithermenu::refresh(void) {
  setItemChecked(bt::NoDither,
                 bt::Image::ditherMode() == bt::NoDither);
  setItemChecked(bt::OrderedDither,
                 bt::Image::ditherMode() == bt::OrderedDither);
  setItemChecked(bt::FloydSteinbergDither,
                 bt::Image::ditherMode() == bt::FloydSteinbergDither);
}


void ConfigDithermenu::itemClicked(unsigned int id, unsigned int) {
  bt::Image::setDitherMode((bt::DitherMode) id);
}

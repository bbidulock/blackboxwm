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

#include "Configmenu.hh"
#include "Screen.hh"
#include "../nls/blackbox-nls.hh"

#include <Image.hh>
#include <i18n.hh>


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


enum {
  FocusModel,
  WindowPlacement,
  ImageDithering,
  OpaqueWindowMoving,
  FullMaximization,
  FocusNewWindows,
  FocusLastWindowOnWorkspace,
  DisableBindings,
  DisableToolbar,
  ClickToFocus,
  SloppyFocus,
  AutoRaise,
  ClickRaise,
  IgnoreShadedWindows
};

Configmenu::Configmenu(bt::Application &app, unsigned int screen,
                       BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen) {
  setTitle(bt::i18n(ConfigmenuSet, ConfigmenuConfigOptions, "Config options"));
  showTitle();

  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuFocusModel,
                      "Focus Model"),
             new ConfigFocusmenu(app, screen, bscreen),
             FocusModel);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuWindowPlacement,
                      "Window Placement"),
             new ConfigPlacementmenu(app, screen, bscreen),
             WindowPlacement);

  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuImageDithering,
                      "Image Dithering"),
             new ConfigDithermenu(app, screen),
             ImageDithering);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuOpaqueMove,
                      "Opaque Window Moving"),
             OpaqueWindowMoving);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuFullMax,
                      "Full Maximization"),
             FullMaximization);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuFocusNew,
                      "Focus New Windows"),
             FocusNewWindows);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuFocusLast,
                      "Focus Last Window on Workspace"),
             FocusLastWindowOnWorkspace);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuDisableBindings,
                      "Disable Bindings with Scroll Lock"),
             DisableBindings);
  insertItem("Disable Toolbar", DisableToolbar);
}


void Configmenu::refresh(void) {
  ScreenResource& res = _bscreen->resource();
  setItemChecked(OpaqueWindowMoving, res.doOpaqueMove());
  setItemChecked(FullMaximization, res.doFullMax());
  setItemChecked(FocusNewWindows, res.doFocusNew());
  setItemChecked(FocusLastWindowOnWorkspace, res.doFocusLast());
  setItemChecked(DisableBindings, res.allowScrollLock());
  setItemChecked(DisableToolbar, _bscreen->toolbar() == 0);
}


void Configmenu::itemClicked(unsigned int id, unsigned int) {
  ScreenResource& res = _bscreen->resource();
  switch (id) {
  case OpaqueWindowMoving: // opaque move
    res.saveOpaqueMove(! res.doOpaqueMove());
    break;

  case FullMaximization: // full maximization
    res.saveFullMax(! res.doFullMax());
    break;

  case FocusNewWindows: // focus new windows
    res.saveFocusNew(! res.doFocusNew());
    break;

  case FocusLastWindowOnWorkspace: // focus last window on workspace
    res.saveFocusLast(! res.doFocusLast());
    break;

  case DisableBindings: // disable keybindings with Scroll Lock
    res.saveAllowScrollLock(! res.allowScrollLock());
    _bscreen->reconfigure();
    break;

  case DisableToolbar:
    if (_bscreen->toolbar())
      _bscreen->destroyToolbar();
    else
      _bscreen->createToolbar();
    break;

  default:
    return;
  } // switch
  _bscreen->saveResource();
}


ConfigFocusmenu::ConfigFocusmenu(bt::Application &app, unsigned int screen,
                                 BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen) {
  setTitle(bt::i18n(ConfigmenuSet, ConfigmenuFocusModel, "Focus Model"));
  showTitle();

  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuClickToFocus, "Click To Focus"),
             ClickToFocus);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuSloppyFocus, "Sloppy Focus"),
             SloppyFocus);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuAutoRaise, "Auto Raise"),
             AutoRaise);
  insertItem(bt::i18n(ConfigmenuSet, ConfigmenuClickRaise, "Click Raise"),
             ClickRaise);
}


void ConfigFocusmenu::refresh(void) {
  ScreenResource& res = _bscreen->resource();
  setItemChecked(ClickToFocus, !res.isSloppyFocus());
  setItemChecked(SloppyFocus, res.isSloppyFocus());
  setItemEnabled(AutoRaise, res.isSloppyFocus());
  setItemChecked(AutoRaise, res.doAutoRaise());
  setItemEnabled(ClickRaise, res.isSloppyFocus());
  setItemChecked(ClickRaise, res.doClickRaise());
}


void ConfigFocusmenu::itemClicked(unsigned int id, unsigned int) {
  ScreenResource& res = _bscreen->resource();
  switch (id) {
  case ClickToFocus:
    _bscreen->toggleFocusModel(BScreen::ClickToFocus);
    break;

  case SloppyFocus:
    _bscreen->toggleFocusModel(BScreen::SloppyFocus);
    break;

  case AutoRaise: // auto raise with sloppy focus
    res.saveAutoRaise(! res.doAutoRaise());
    break;

  case ClickRaise: // click raise with sloppy focus
    res.saveClickRaise(! res.doClickRaise());
    // make sure the appropriate mouse buttons are grabbed on the windows
    _bscreen->toggleFocusModel(BScreen::SloppyFocus);
    break;
  default:
    return;
  } // switch
  _bscreen->saveResource();
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

  insertSeparator();

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

  insertSeparator();

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

  case IgnoreShadedWindows:
    res.savePlacementIgnoresShaded(! res.placementIgnoresShaded());
    break;

  default:
    return;
  } // switch
  _bscreen->saveResource();
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


void ConfigDithermenu::itemClicked(unsigned int id, unsigned int)
{ bt::Image::setDitherMode((bt::DitherMode) id); }

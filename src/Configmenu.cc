// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Configmenu.cc for Blackbox - An X11 Window Manager
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

#include "gettext.h"
#include "Configmenu.hh"
#include "Screen.hh"
#include "Slitmenu.hh"
#include "Toolbarmenu.hh"

#include <Image.hh>
#include <Unicode.hh>


class ConfigFocusmenu : public bt::Menu {
public:
  ConfigFocusmenu(bt::Application &app, unsigned int screen,
                  BScreen *bscreen);

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
  ConfigDithermenu(bt::Application &app, unsigned int screen,
                   BScreen *bscreen);

  void refresh(void);

protected:
  void itemClicked(unsigned int id, unsigned int);

private:
  BScreen *_bscreen;
};


enum {
  FocusModel,
  WindowPlacement,
  ImageDithering,
  OpaqueWindowMoving,
  OpaqueWindowResizing,
  FullMaximization,
  FocusNewWindows,
  FocusLastWindowOnWorkspace,
  ChangeWorkspaceWithMouseWheel,
  ShadeWindowWithMouseWheel,
  ToolbarActionsWithMouseWheel,
  DisableBindings,
  ToolbarOptions,
  SlitOptions,
  ClickToFocus,
  SloppyFocus,
  AutoRaise,
  ClickRaise,
  IgnoreShadedWindows
};

Configmenu::Configmenu(bt::Application &app, unsigned int screen,
                       BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen)
{
  setAutoDelete(false);
// TRANS The name of the primary window manager configuration menu.
  setTitle(bt::toUnicode(gettext("Configuration Options")));
  showTitle();

  ConfigFocusmenu *focusmenu =
    new ConfigFocusmenu(app, screen, bscreen);
  ConfigPlacementmenu *placementmenu =
    new ConfigPlacementmenu(app, screen, bscreen);
  ConfigDithermenu *dithermenu =
    new ConfigDithermenu(app, screen, bscreen);

// TRANS The name of the submenu for controlling the window focus model.
  insertItem(bt::toUnicode(gettext("Focus Model")), focusmenu, FocusModel);
// TRANS The name of the submenu for controlling automatic window placement.
  insertItem(bt::toUnicode(gettext("Window Placement")),
             placementmenu, WindowPlacement);
// TRANS The name of the submenu for controlling image dithering.
  insertItem(bt::toUnicode(gettext("Image Dithering")), dithermenu, ImageDithering);
  insertSeparator();
// TRANS When windows are moved they are moved opaquely (their contents show).
  insertItem(bt::toUnicode(gettext("Opaque Window Moving")), OpaqueWindowMoving);
// TRANS When windows are resized they are resized opaquely (their contents show).
  insertItem(bt::toUnicode(gettext("Opaque Window Resizing")), OpaqueWindowResizing);
// TRANS When window are maximized, they are maximized to the full screen size.
  insertItem(bt::toUnicode(gettext("Full Maximization")), FullMaximization);
// TRANS When new windows appear on the screen, they are focussed immediately.
  insertItem(bt::toUnicode(gettext("Focus New Windows")), FocusNewWindows);
// TRANS When changing to a workspace, the previous focussed window on that workspace is focussed.
  insertItem(bt::toUnicode(gettext("Focus Last Window on Workspace")),
             FocusLastWindowOnWorkspace);
// TRANS The workspaces may be changed using the mouse wheel.
  insertItem(bt::toUnicode(gettext("Change Workspace with Mouse Wheel")),
             ChangeWorkspaceWithMouseWheel);
// TRANS The mouse wheel on the window title bar shades or unshades a window.
  insertItem(bt::toUnicode(gettext("Shade Windows with Mouse Wheel")),
             ShadeWindowWithMouseWheel);
// TRANS The toolbar responds to mouse wheel actions.
  insertItem(bt::toUnicode(gettext("Toolbar Actions with Mouse Wheel")),
             ToolbarActionsWithMouseWheel);
// TRANS Disable global key binding by setting the Scroll Lock key.
  insertItem(bt::toUnicode(gettext("Disable Bindings with Scroll Lock")),
             DisableBindings);
  insertSeparator();
// TRANS The name of the submenu for controlling toolbar options.
  insertItem(bt::toUnicode(gettext("Toolbar Options")),
             bscreen->toolbarmenu(), ToolbarOptions);
// TRANS The name of the submenu for controlling slit options.
  insertItem(bt::toUnicode(gettext("Slit Options")), bscreen->slitmenu(), SlitOptions);
}


void Configmenu::refresh(void) {
  const BlackboxResource &res = _bscreen->blackbox()->resource();
  setItemChecked(OpaqueWindowMoving, res.opaqueMove());
  setItemChecked(OpaqueWindowResizing, res.opaqueResize());
  setItemChecked(FullMaximization, res.fullMaximization());
  setItemChecked(FocusNewWindows, res.focusNewWindows());
  setItemChecked(FocusLastWindowOnWorkspace, res.focusLastWindowOnWorkspace());
  setItemChecked(ChangeWorkspaceWithMouseWheel,
                 res.changeWorkspaceWithMouseWheel());
  setItemChecked(ShadeWindowWithMouseWheel,
                 res.shadeWindowWithMouseWheel());
  setItemChecked(ToolbarActionsWithMouseWheel,
                 res.toolbarActionsWithMouseWheel());
  setItemChecked(DisableBindings, res.allowScrollLock());
}


void Configmenu::itemClicked(unsigned int id, unsigned int) {
  BlackboxResource &res = _bscreen->blackbox()->resource();
  switch (id) {
  case OpaqueWindowMoving: // opaque move
    res.setOpaqueMove(!res.opaqueMove());
    break;

  case OpaqueWindowResizing:
    res.setOpaqueResize(!res.opaqueResize());
    break;

  case FullMaximization: // full maximization
    res.setFullMaximization(!res.fullMaximization());
    break;

  case FocusNewWindows: // focus new windows
    res.setFocusNewWindows(!res.focusNewWindows());
    break;

  case FocusLastWindowOnWorkspace: // focus last window on workspace
    res.setFocusLastWindowOnWorkspace(!res.focusLastWindowOnWorkspace());
    break;

  case ChangeWorkspaceWithMouseWheel:
    res.setChangeWorkspaceWithMouseWheel(!res.changeWorkspaceWithMouseWheel());
    break;

  case ShadeWindowWithMouseWheel:
    res.setShadeWindowWithMouseWheel(!res.shadeWindowWithMouseWheel());
    break;

  case ToolbarActionsWithMouseWheel:
    res.setToolbarActionsWithMouseWheel(!res.toolbarActionsWithMouseWheel());
    break;

  case DisableBindings: // disable keybindings with Scroll Lock
    res.setAllowScrollLock(!res.allowScrollLock());
    _bscreen->blackbox()->reconfigure();
    break;

  default:
    return;
  } // switch

  res.save(*_bscreen->blackbox());
}


ConfigFocusmenu::ConfigFocusmenu(bt::Application &app, unsigned int screen,
                                 BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen)
{
// TRANS The name of the submenu for controlling the window focus model.
  setTitle(bt::toUnicode(gettext("Focus Model")));
  showTitle();

// TRANS The user must click on a window to focus it.
  insertItem(bt::toUnicode(gettext("Click to Focus")), ClickToFocus);
// TRANS The user can move the mouse into the window to focus it.
  insertItem(bt::toUnicode(gettext("Sloppy Focus")), SloppyFocus);
// TRANS When a window is focused it is automatically raised.
  insertItem(bt::toUnicode(gettext("Auto Raise")), AutoRaise);
// TRANS When a window is clicked it is raised.
  insertItem(bt::toUnicode(gettext("Click Raise")), ClickRaise);
}


void ConfigFocusmenu::refresh(void) {
  const BlackboxResource &res = _bscreen->blackbox()->resource();

  setItemChecked(ClickToFocus, res.focusModel() == ClickToFocusModel);
  setItemChecked(SloppyFocus, res.focusModel() == SloppyFocusModel);

  setItemEnabled(AutoRaise, res.focusModel() == SloppyFocusModel);
  setItemChecked(AutoRaise, res.autoRaise());

  setItemEnabled(ClickRaise, res.focusModel() == SloppyFocusModel);
  setItemChecked(ClickRaise, res.clickRaise());
}


void ConfigFocusmenu::itemClicked(unsigned int id, unsigned int) {
  BlackboxResource &res = _bscreen->blackbox()->resource();
  switch (id) {
  case ClickToFocus:
    _bscreen->toggleFocusModel(ClickToFocusModel);
    break;

  case SloppyFocus:
    _bscreen->toggleFocusModel(SloppyFocusModel);
    break;

  case AutoRaise: // auto raise with sloppy focus
    res.setAutoRaise(!res.autoRaise());
    break;

  case ClickRaise: // click raise with sloppy focus
    res.setClickRaise(!res.clickRaise());
    // make sure the appropriate mouse buttons are grabbed on the windows
    _bscreen->toggleFocusModel(SloppyFocusModel);
    break;

  default: return;
  } // switch
  res.save(*_bscreen->blackbox());
}


ConfigPlacementmenu::ConfigPlacementmenu(bt::Application &app,
                                         unsigned int screen,
                                         BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen)
{
// TRANS The name of the submenu for controlling automatic window placement.
  setTitle(bt::toUnicode(gettext("Window Placement")));
  showTitle();

// TRANS Arrange windows intelligently in rows.
  insertItem(bt::toUnicode(gettext("Smart Placement (Rows)")), RowSmartPlacement);
// TRANS Arrange windows intelligently in columns.
  insertItem(bt::toUnicode(gettext("Smart Placement (Columns)")), ColSmartPlacement);
// TRANS Center windows.
  insertItem(bt::toUnicode(gettext("Center Placement")), CenterPlacement);
// TRANS Arrange windows in a cascade.
  insertItem(bt::toUnicode(gettext("Cascade Placement")), CascadePlacement);

  insertSeparator();

// TRANS When arranging windows, arrange from left to right.
  insertItem(bt::toUnicode(gettext("Left to Right")), LeftRight);
// TRANS When arranging windows, arrange from right to left.
  insertItem(bt::toUnicode(gettext("Right to Left")), RightLeft);
// TRANS When arranging windows, arrange from top to bottom.
  insertItem(bt::toUnicode(gettext("Top to Bottom")), TopBottom);
// TRANS When arranging windows, arrange from bottom to top.
  insertItem(bt::toUnicode(gettext("Bottom to Top")), BottomTop);

  insertSeparator();

// TRANS When arranging windows, ignore the presence of shaded windows.
  insertItem(bt::toUnicode(gettext("Ignore Shaded Windows")), IgnoreShadedWindows);
}


void ConfigPlacementmenu::refresh(void) {
  const BlackboxResource &res = _bscreen->blackbox()->resource();
  bool rowsmart = res.windowPlacementPolicy() == RowSmartPlacement,
       colsmart = res.windowPlacementPolicy() == ColSmartPlacement,
         center = res.windowPlacementPolicy() == CenterPlacement,
        cascade = res.windowPlacementPolicy() == CascadePlacement,
             rl = res.rowPlacementDirection() == LeftRight,
             tb = res.colPlacementDirection() == TopBottom;

  setItemChecked(RowSmartPlacement, rowsmart);
  setItemChecked(ColSmartPlacement, colsmart);
  setItemChecked(CenterPlacement, center);
  setItemChecked(CascadePlacement, cascade);

  setItemEnabled(LeftRight, !center && !cascade);
  setItemChecked(LeftRight, !center && (cascade || rl));

  setItemEnabled(RightLeft, !center && !cascade);
  setItemChecked(RightLeft, !center && (!cascade && !rl));

  setItemEnabled(TopBottom, !center && !cascade);
  setItemChecked(TopBottom, !center && (cascade || tb));

  setItemEnabled(BottomTop, !center && !cascade);
  setItemChecked(BottomTop, !center && (!cascade && !tb));

  setItemEnabled(IgnoreShadedWindows, !center);
  setItemChecked(IgnoreShadedWindows, !center && res.placementIgnoresShaded());
}


void ConfigPlacementmenu::itemClicked(unsigned int id, unsigned int) {
  BlackboxResource &res = _bscreen->blackbox()->resource();
  switch (id) {
  case RowSmartPlacement:
  case ColSmartPlacement:
  case CenterPlacement:
  case CascadePlacement:
    res.setWindowPlacementPolicy(id);
    break;

  case LeftRight:
  case RightLeft:
    res.setRowPlacementDirection(id);
    break;

  case TopBottom:
  case BottomTop:
    res.setColPlacementDirection(id);
    break;

  case IgnoreShadedWindows:
    res.setPlacementIgnoresShaded(! res.placementIgnoresShaded());
    break;

  default:
    return;
  } // switch
  res.save(*_bscreen->blackbox());
}


ConfigDithermenu::ConfigDithermenu(bt::Application &app, unsigned int screen,
                                   BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen)
{
// TRANS The name of the submenu for controlling image dithering.
  setTitle(bt::toUnicode(gettext("Image Dithering")));
  showTitle();

// TRANS Do not dither images.
  insertItem(bt::toUnicode(gettext("Do not dither images")), bt::NoDither);
// TRANS Use a fast dither algorithm for dithering images.
  insertItem(bt::toUnicode(gettext("Use fast dither")), bt::OrderedDither);
// TRANS Use a high-quality dither algorithm for dithering images.
  insertItem(bt::toUnicode(gettext("Use high-quality dither")), bt::FloydSteinbergDither);
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
  _bscreen->blackbox()->resource().save(*_bscreen->blackbox());
}

// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Toolbarmenu.cc for Blackbox - an X11 Window manager
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
#include "Toolbarmenu.hh"
#include "Toolbar.hh"

#include <Unicode.hh>


class ToolbarPlacementmenu : public bt::Menu {
public:
  ToolbarPlacementmenu(bt::Application &app, unsigned int screen,
                       BScreen *bscreen);

  void refresh(void);

protected:
  void itemClicked(unsigned int id, unsigned int button);

private:
  BScreen *_bscreen;
};


enum {
  EnableToolbar,
  Placement,
  AlwaysOnTop,
  AutoHide
};


Toolbarmenu::Toolbarmenu(bt::Application &app, unsigned int screen,
                         BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen)
{
  setTitle(bt::toUnicode(gettext("Toolbar Options")));
  showTitle();

  ToolbarPlacementmenu *menu = new ToolbarPlacementmenu(app, screen, bscreen);
  insertItem(bt::toUnicode(gettext("Enable Toolbar")), EnableToolbar);
  insertSeparator();
// TRANS The name of the placement submenu of the toolbar options menu
// TRANS that allows the user to specify where they want the toolbar stacked
// TRANS with relation to other windows.
  insertItem(bt::toUnicode(gettext("Placement")), menu, Placement);
// TRANS Always place the toolbar above other windows on the desktop.
  insertItem(bt::toUnicode(gettext("Always on top")), AlwaysOnTop);
// TRANS Automatically hide the toolbar when the pointer leaves the toolbar.
  insertItem(bt::toUnicode(gettext("Auto Hide")), AutoHide);
}


void Toolbarmenu::refresh(void) {
  const ToolbarOptions &options = _bscreen->resource().toolbarOptions();
  setItemChecked(EnableToolbar, options.enabled);
  setItemChecked(AlwaysOnTop, options.always_on_top);
  setItemChecked(AutoHide, options.auto_hide);
}


void Toolbarmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1)
    return;

  Toolbar *toolbar = _bscreen->toolbar();
  ToolbarOptions &options =
    const_cast<ToolbarOptions &>(_bscreen->resource().toolbarOptions());

  switch (id) {
  case EnableToolbar:
    options.enabled = (toolbar == 0);
    _bscreen->saveResource();
    if (toolbar != 0)
      _bscreen->destroyToolbar();
    else
      _bscreen->createToolbar();
    break;

  case AlwaysOnTop:
    options.always_on_top = !options.always_on_top;
    _bscreen->saveResource();
    if (toolbar) {
      StackingList::Layer new_layer = (options.always_on_top
                                       ? StackingList::LayerAbove
                                       : StackingList::LayerNormal);
      _bscreen->stackingList().changeLayer(toolbar, new_layer);
      _bscreen->restackWindows();
    }
    break;

  case AutoHide:
    options.auto_hide = !options.auto_hide;
    _bscreen->saveResource();
    if (toolbar)
      toolbar->toggleAutoHide();
    break;

  default:
    break;
  } // switch
}


ToolbarPlacementmenu::ToolbarPlacementmenu(bt::Application &app,
                                           unsigned int screen,
                                           BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen)
{
// TRANS The name of the menu that controls toolbar placement; i.e.,
// TRANS which corner or edge of the screen on which the toolbar is placed.
  setTitle(bt::toUnicode(gettext("Toolbar Placement")));
  showTitle();

// TRANS Top left corner of the screen.
  insertItem(bt::toUnicode(gettext("Top Left")),      Toolbar::TopLeft);
// TRANS Centered at the top of the screen.
  insertItem(bt::toUnicode(gettext("Top Center")),    Toolbar::TopCenter);
// TRANS Top right corner of the screen.
  insertItem(bt::toUnicode(gettext("Top Right")),     Toolbar::TopRight);
  insertSeparator();
// TRANS Bottom left corner of the screen.
  insertItem(bt::toUnicode(gettext("Bottom Left")),   Toolbar::BottomLeft);
// TRANS Centered at the bottom of the screen.
  insertItem(bt::toUnicode(gettext("Bottom Center")), Toolbar::BottomCenter);
// TRANS Bottom right corner of the screen.
  insertItem(bt::toUnicode(gettext("Bottom Right")),  Toolbar::BottomRight);
}


void ToolbarPlacementmenu::refresh(void) {
  const ToolbarOptions &options = _bscreen->resource().toolbarOptions();
  setItemChecked(Toolbar::TopLeft,
                 options.placement == Toolbar::TopLeft);
  setItemChecked(Toolbar::TopCenter,
                 options.placement == Toolbar::TopCenter);
  setItemChecked(Toolbar::TopRight,
                 options.placement == Toolbar::TopRight);
  setItemChecked(Toolbar::BottomLeft,
                 options.placement == Toolbar::BottomLeft);
  setItemChecked(Toolbar::BottomCenter,
                 options.placement == Toolbar::BottomCenter);
  setItemChecked(Toolbar::BottomRight,
                 options.placement == Toolbar::BottomRight);
}


void ToolbarPlacementmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1)
    return;

  Toolbar *toolbar = _bscreen->toolbar();
  ToolbarOptions &options =
    const_cast<ToolbarOptions &>(_bscreen->resource().toolbarOptions());

  options.placement = id;
  if (toolbar)
    toolbar->setPlacement((Toolbar::Placement) id);
  _bscreen->saveResource();
}

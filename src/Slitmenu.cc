// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Slitmenu.cc for Blackbox - an X11 Window Manager
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

#include "Slitmenu.hh"
#include "Screen.hh"
#include "Slit.hh"

#include <Unicode.hh>


class SlitDirectionmenu : public bt::Menu {
public:
  SlitDirectionmenu(bt::Application &app, unsigned int screen,
		    BScreen *bscreen);

  void refresh(void);

protected:
  void itemClicked(unsigned int id, unsigned int button);

private:
  BScreen *_bscreen;
};


class SlitPlacementmenu : public bt::Menu {
public:
 SlitPlacementmenu(bt::Application &app, unsigned int screen,
		   BScreen *bscreen);

  void refresh(void);

protected:
  void itemClicked(unsigned int id, unsigned int button);

private:
  BScreen *_bscreen;
};


enum {
  Direction,
  Placement,
  AlwaysOnTop,
  AutoHide
};

Slitmenu::Slitmenu(bt::Application &app, unsigned int screen, BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen)
{
  setTitle(bt::toUnicode("Slit Options"));
  showTitle();

  insertItem(bt::toUnicode("Direction"), new SlitDirectionmenu(app, screen, bscreen),
             Direction);
  insertItem(bt::toUnicode("Placement"), new SlitPlacementmenu(app, screen, bscreen),
             Placement);
  insertSeparator();
  insertItem(bt::toUnicode("Always on top"), AlwaysOnTop);
  insertItem(bt::toUnicode("Auto hide"), AutoHide);
}


void Slitmenu::refresh(void) {
  const SlitOptions &options = _bscreen->resource().slitOptions();
  setItemChecked(AlwaysOnTop, options.always_on_top);
  setItemChecked(AutoHide, options.auto_hide);
}


void Slitmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1)
    return;

  Slit *slit = _bscreen->slit();
  SlitOptions &options =
    const_cast<SlitOptions &>(_bscreen->resource().slitOptions());

  switch (id) {
  case AlwaysOnTop:
    options.always_on_top = !options.always_on_top;
    _bscreen->saveResource();
    if (slit) {
      StackingList::Layer new_layer = (options.always_on_top
                                       ? StackingList::LayerAbove
                                       : StackingList::LayerNormal);
      _bscreen->stackingList().changeLayer(slit, new_layer);
      _bscreen->restackWindows();
    }
    break;

  case AutoHide:
    options.auto_hide = !options.auto_hide;
    _bscreen->saveResource();
    if (slit)
      slit->toggleAutoHide();
    break;

  default:
    return;
  } // switch
}


SlitDirectionmenu::SlitDirectionmenu(bt::Application &app, unsigned int screen,
                                     BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen)
{
  setTitle(bt::toUnicode("Slit Direction"));
  showTitle();

  insertItem(bt::toUnicode("Horizontal"), Slit::Horizontal);
  insertItem(bt::toUnicode("Vertical"), Slit::Vertical);
}


void SlitDirectionmenu::refresh(void) {
  const SlitOptions &options =_bscreen->resource().slitOptions();
  setItemChecked(Slit::Horizontal, options.direction == Slit::Horizontal);
  setItemChecked(Slit::Vertical, options.direction == Slit::Vertical);
}


void SlitDirectionmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1)
    return;

  Slit *slit = _bscreen->slit();
  SlitOptions &options =
    const_cast<SlitOptions &>(_bscreen->resource().slitOptions());

  options.direction = id;
  _bscreen->saveResource();
  if (slit)
    slit->reconfigure();
}


SlitPlacementmenu::SlitPlacementmenu(bt::Application &app, unsigned int screen,
                                     BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen)
{
  setTitle(bt::toUnicode("Slit Placement"));
  showTitle();

  insertItem(bt::toUnicode("Top Left"),      Slit::TopLeft);
  insertItem(bt::toUnicode("Center Left"),   Slit::CenterLeft);
  insertItem(bt::toUnicode("Bottom Left"),   Slit::BottomLeft);
  insertSeparator();
  insertItem(bt::toUnicode("Top Center"),    Slit::TopCenter);
  insertItem(bt::toUnicode("Bottom Center"), Slit::BottomCenter);
  insertSeparator();
  insertItem(bt::toUnicode("Top Right"),     Slit::TopRight);
  insertItem(bt::toUnicode("Center Right"),  Slit::CenterRight);
  insertItem(bt::toUnicode("Bottom Right"),  Slit::BottomRight);
}


void SlitPlacementmenu::refresh(void) {
  const SlitOptions &options = _bscreen->resource().slitOptions();
  setItemChecked(Slit::TopLeft,      options.placement == Slit::TopLeft);
  setItemChecked(Slit::CenterLeft,   options.placement == Slit::CenterLeft);
  setItemChecked(Slit::BottomLeft,   options.placement == Slit::BottomLeft);
  setItemChecked(Slit::TopCenter,    options.placement == Slit::TopCenter);
  setItemChecked(Slit::BottomCenter, options.placement == Slit::BottomCenter);
  setItemChecked(Slit::TopRight,     options.placement == Slit::TopRight);
  setItemChecked(Slit::CenterRight,  options.placement == Slit::CenterRight);
  setItemChecked(Slit::BottomRight,  options.placement == Slit::BottomRight);
}


void SlitPlacementmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1)
    return;

  Slit *slit = _bscreen->slit();
  SlitOptions &options =
    const_cast<SlitOptions &>(_bscreen->resource().slitOptions());

  options.placement = id;
  _bscreen->saveResource();
  if (slit)
    slit->reconfigure();
}

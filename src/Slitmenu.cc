// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Slitmenu.cc for Blackbox - an X11 Window Manager
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

#include "Slitmenu.hh"
#include "Screen.hh"
#include "Slit.hh"


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
  setTitle("Slit Options");
  showTitle();

  insertItem("Direction", new SlitDirectionmenu(app, screen, bscreen),
             Direction);
  insertItem("Placement", new SlitPlacementmenu(app, screen, bscreen),
             Placement);
  insertSeparator();
  insertItem("Always on top", AlwaysOnTop);
  insertItem("Auto hide", AutoHide);
}


void Slitmenu::refresh(void) {
  ScreenResource& res = _bscreen->resource();
  setItemChecked(AlwaysOnTop, res.isSlitOnTop());
  setItemChecked(AutoHide, res.doSlitAutoHide());
}


void Slitmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  ScreenResource& res = _bscreen->resource();
  Slit *slit = _bscreen->slit();

  switch (id) {
  case AlwaysOnTop:
    res.saveSlitOnTop(!res.isSlitOnTop());
    _bscreen->saveResource();
    if (slit) {
      _bscreen->changeLayer(slit, (res.isSlitOnTop()
                                   ? StackingList::LayerAbove
                                   : StackingList::LayerNormal));
    }
    break;

  case AutoHide:
    res.saveSlitAutoHide(!res.doSlitAutoHide());
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
  setTitle("Slit Direction");
  showTitle();

  insertItem("Horizontal", Slit::Horizontal);
  insertItem("Vertical", Slit::Vertical);
}


void SlitDirectionmenu::refresh(void) {
  ScreenResource& res = _bscreen->resource();
  setItemChecked(Slit::Horizontal, res.slitDirection() == Slit::Horizontal);
  setItemChecked(Slit::Vertical, res.slitDirection() == Slit::Vertical);
}


void SlitDirectionmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  ScreenResource& res = _bscreen->resource();
  Slit *slit = _bscreen->slit();

  res.saveSlitDirection((Slit::Direction) id);
  if (slit) slit->reconfigure();
  _bscreen->saveResource();
}


SlitPlacementmenu::SlitPlacementmenu(bt::Application &app, unsigned int screen,
                                     BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen)
{
  setTitle("Slit Placement");
  showTitle();

  insertItem("Top Left",      Slit::TopLeft);
  insertItem("Center Left",   Slit::CenterLeft);
  insertItem("Bottom Left",   Slit::BottomLeft);
  insertSeparator();
  insertItem("Top Center",    Slit::TopCenter);
  insertItem("Bottom Center", Slit::BottomCenter);
  insertSeparator();
  insertItem("Top Right",     Slit::TopRight);
  insertItem("Center Right",  Slit::CenterRight);
  insertItem("Bottom Right",  Slit::BottomRight);
}


void SlitPlacementmenu::refresh(void) {
  int placement = _bscreen->resource().slitPlacement();
  setItemChecked(Slit::TopLeft,      placement == Slit::TopLeft);
  setItemChecked(Slit::CenterLeft,   placement == Slit::CenterLeft);
  setItemChecked(Slit::BottomLeft,   placement == Slit::BottomLeft);
  setItemChecked(Slit::TopCenter,    placement == Slit::TopCenter);
  setItemChecked(Slit::BottomCenter, placement == Slit::BottomCenter);
  setItemChecked(Slit::TopRight,     placement == Slit::TopRight);
  setItemChecked(Slit::CenterRight,  placement == Slit::CenterRight);
  setItemChecked(Slit::BottomRight,  placement == Slit::BottomRight);
}


void SlitPlacementmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  ScreenResource& res = _bscreen->resource();
  Slit *slit = _bscreen->slit();

  res.saveSlitPlacement((Slit::Placement) id);
  if (slit) slit->reconfigure();
  _bscreen->saveResource();
}

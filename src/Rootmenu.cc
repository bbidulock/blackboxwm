// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Rootmenu.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2004 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2004
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

#include "Rootmenu.hh"
#include "Screen.hh"

#include <Unicode.hh>


Rootmenu::Rootmenu(bt::Application &app, unsigned int screen, BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen) { }


void Rootmenu::insertFunction(const bt::ustring &label,
                              unsigned int function,
                              const std::string &exec,
                              unsigned int id,
                              unsigned int index) {
  unsigned int x = insertItem(label, id, index);
  _funcmap.insert(FunctionMap::value_type(x, _function(function, exec)));
}


void Rootmenu::clear(void) {
  bt::Menu::clear();
  _funcmap.clear();
}


void Rootmenu::itemClicked(unsigned int id, unsigned int) {
  FunctionMap::const_iterator it = _funcmap.find(id);
  if (it == _funcmap.end()) return;

  switch (it->second.func) {
  case BScreen::Execute:
    if (! it->second.string.empty())
      bt::bexec(it->second.string, _bscreen->screenInfo().displayString());
    break;

  case BScreen::Restart:
    _bscreen->blackbox()->restart();
    break;

  case BScreen::RestartOther:
    if (! it->second.string.empty())
      _bscreen->blackbox()->restart(it->second.string);
    break;

  case BScreen::Exit:
    _bscreen->blackbox()->quit();
    break;

  case BScreen::SetStyle:
    if (! it->second.string.empty())
      _bscreen->blackbox()->resource().saveStyleFilename(it->second.string);

  case BScreen::Reconfigure:
    _bscreen->blackbox()->reconfigure();
    return;
  } // switch
}

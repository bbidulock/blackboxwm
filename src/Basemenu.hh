// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Basemenu.hh for Blackbox - an X11 Window manager
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

#ifndef   __Basemenu_hh
#define   __Basemenu_hh

#include "Widget.hh"

#include <list>
#include <string>
using std::string;


class Basemenu : public Widget
{
public:
  enum Function { Submenu = -2, Custom = -1 };

  class Item {
  public:
    enum Type { Default, Separator };
    Item(Type t = Default)
      : def(t == Default), sep(t == Separator),
        active(false), title(false), enable(true), checked(false),
        sub(0), fun(Custom), idx(-1), height(0) { }
    Item(int f)
      : def(false), sep(false),
        active(false), title(false), enable(true), checked(false),
        sub(0), fun(f), idx(-1), height(0) { }
    Item(int f, const string &c)
      : def(false), sep(false),
        active(false), title(false), enable(true), checked(false),
        sub(0), fun(f), idx(-1), height(0), cmd(c) { }
    Item(Basemenu *s)
      : def(false), sep(false),
        active(false), title(false), enable(true), checked(false),
        sub(s), fun(Submenu), idx(-1), height(0) { }

    bool isDefault() const { return def; }
    bool isSeparator() const { return sep; }
    bool isTitle() const { return title; }
    bool isActive() const { return active; }
    bool isEnabled() const { return enable; }
    bool isChecked() const { return checked; }
    Basemenu *submenu() const { return sub; }
    int function() const { return fun; }
    int index() const { return idx; }
    const string &label() const { return lbl; }
    const string &command() const { return cmd; }

  private:
    unsigned int     def : 1;
    unsigned int     sep : 1;
    unsigned int  active : 1;
    unsigned int   title : 1;
    unsigned int  enable : 1;
    unsigned int checked : 1;
    Basemenu *sub;
    int fun;
    int idx;
    int height;
    string lbl;
    string cmd;
    friend class Basemenu;
  };

  Basemenu(int scr);
  virtual ~Basemenu();

  int insert(const string &label, const Item &item = Item::Default, int index = -1);
  int insertSeparator() { return insert(string(), Item::Separator); }
  void change(int index, const string &label, const Item &item = Item::Default);
  void remove(int index);

  bool autoDelete() const { return auto_delete; }
  void setAutoDelete(bool ad) { auto_delete = ad; }
  void clear();

  int count() const { return items.size() - (show_title ? 1 : 0); }

  void setItemEnabled(int, bool);
  bool isItemEnabled(int) const;

  void setItemChecked(int, bool);
  bool isItemChecked(int) const;

  void showTitle();
  void hideTitle();

  virtual void popup(int, int, bool = true);
  virtual void popup(const Point &, bool = true);
  virtual void hide();

  virtual void refresh() { }
  virtual void reconfigure();

protected:
  virtual void setActiveItem(int);
  virtual void setActiveItem(const Rect &, Item &);
  virtual void showActiveSubmenu();
  virtual void showSubmenu(const Rect &, const Item &);
  virtual void updateSize();

  virtual void buttonPressEvent(XEvent *);
  virtual void buttonReleaseEvent(XEvent *);
  virtual void pointerMotionEvent(XEvent *);
  virtual void leaveEvent(XEvent *);
  virtual void exposeEvent(XEvent *);
  virtual void keyPressEvent(XEvent *);

  virtual void titleClicked(const Point &, int);
  virtual void itemClicked(const Point &, const Item &, int);

  virtual void hideAll();

private:
  void drawTitle();
  void drawItem(const Rect &, const Item &);
  void clickActiveItem();

  Pixmap title_pixmap, items_pixmap, highlight_pixmap;
  Rect title_rect;
  Rect items_rect;
  typedef std::list<Item> Items;
  Items items;
  Basemenu *parent_menu, *current_submenu;
  int motion;
  int rows, cols;
  int itemw;
  int indent;
  int active_item;
  bool show_title;
  bool size_dirty;
  bool pressed;
  bool title_pressed;
  bool auto_delete;
};

#endif // __Basemenu_hh

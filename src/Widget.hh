// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Widget.hh for Blackbox - an X11 Window manager
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

#ifndef WIDGET_HH
#define WIDGET_HH

#include <X11/Xlib.h>

#include "LinkedList.hh"
#include "Util.hh"

#include <map>
#include <string>
using std::string;

class BColor;
class Widget;

typedef std::map<Window,Widget*> Mapper;


class Widget
{
public:
  enum Type { Normal, Popup, OverrideRedirect };

  // create a toplevel with this constructor
  Widget( int, Type = Normal );
  // create a child with this constructor
  Widget( Widget * );
  virtual ~Widget();

  Window windowID() const { return win; }

  Widget *parent() const { return _parent; }

  int screen() const { return _screen; }

  Type type() const { return _type; }

  int x() const { return _rect.x(); }
  int y() const { return _rect.y(); }
  Point pos() const { return _rect.pos(); }
  void move( int, int );
  void move( const Point & );

  int width() const { return _rect.width(); }
  int height() const { return _rect.height(); }
  Size size() const { return _rect.size(); }
  virtual void resize( int, int );
  virtual void resize( const Size & );

  const Rect &rect() const { return _rect; }
  virtual void setGeometry( int, int, int, int );
  virtual void setGeometry( const Point &, const Size & );
  virtual void setGeometry( const Rect & );

  bool isVisible() const { return visible; }
  virtual void show();
  virtual void hide();

  bool hasFocus() const { return focused; }
  virtual void setFocus();

  const string &title() const { return _title; }
  virtual void setTitle( const string & );

  bool grabMouse();
  void ungrabMouse();

  bool grabKeyboard();
  void ungrabKeyboard();

  void setBackgroundColor( const BColor & );

protected:
  virtual void buttonPressEvent( XEvent * );
  virtual void buttonReleaseEvent( XEvent * );
  virtual void pointerMotionEvent( XEvent * );
  virtual void keyPressEvent( XEvent * );
  virtual void keyReleaseEvent( XEvent * );
  virtual void configureEvent( XEvent * );
  virtual void mapEvent( XEvent * );
  virtual void unmapEvent( XEvent * );
  virtual void focusInEvent( XEvent * );
  virtual void focusOutEvent( XEvent * );
  virtual void exposeEvent( XEvent * );
  virtual void enterEvent( XEvent * );
  virtual void leaveEvent( XEvent * );

private:
  void create();
  void insertChild( Widget * );
  void removeChild( Widget * );

  Window win;
  Widget *_parent;
  LinkedList<Widget> _children;
  Type _type;
  Rect _rect;
  string _title;
  bool visible;
  bool focused;
  bool grabbedMouse;
  bool grabbedKeyboard;
  int _screen;

  friend class BaseDisplay;
  static Mapper mapper;
};

#endif // WIDGET_HH


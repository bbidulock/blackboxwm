// Slit.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

#ifndef   __Slit_hh
#define   __Slit_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Screen.hh"
#include "Basemenu.hh"
#include "LinkedList.hh"

// forward declaration
class Slit;
class Slitmenu;

class Slitmenu : public Basemenu {
private:
    class Directionmenu : public Basemenu {
    private:
	Slitmenu *slitmenu;

    protected:
	virtual void itemSelected(int, int);

    public:
	Directionmenu(Slitmenu *);
    };

    class Placementmenu : public Basemenu {
    private:
	Slitmenu *slitmenu;

    protected:
	virtual void itemSelected(int, int);

    public:
	Placementmenu(Slitmenu *);
    };

    Directionmenu *directionmenu;
    Placementmenu *placementmenu;

    Slit *slit;

    friend class Directionmenu;
    friend class Placementmenu;
    friend class Slit;


protected:
    virtual void itemSelected(int, int);
    virtual void internal_hide();


public:
    Slitmenu(Slit *);
    virtual ~Slitmenu();

    Basemenu *getDirectionmenu() const { return directionmenu; }
    Basemenu *getPlacementmenu() const { return placementmenu; }

    void reconfigure();
};


class Slit : public TimeoutHandler {
private:
  class SlitClient {
  public:
    Window window, client_window, icon_window;

    int x, y;
    unsigned int width, height;
  };

  Bool on_top, hidden, do_auto_hide;

  Blackbox *blackbox;
  BScreen *screen;
  BTimer *timer;
  NETStrut strut;

  LinkedList<SlitClient> *clientList;
  Slitmenu *slitmenu;

  struct frame {
    Pixmap pixmap;
    Window window;

    int x, y, x_hidden, y_hidden;
    unsigned int width, height;
  } frame;

  friend class Slitmenu;
  friend class Slitmenu::Directionmenu;
  friend class Slitmenu::Placementmenu;


public:
  Slit(BScreen *);
  virtual ~Slit();

  inline const Bool &isOnTop() const { return on_top; }
  inline const Bool &isHidden() const { return hidden; }
  inline const Bool &doAutoHide() const { return do_auto_hide; }

  inline Slitmenu *getMenu() { return slitmenu; }

  inline const Window &getWindowID() const { return frame.window; }

  inline const int &getX() const
  { return ((hidden) ? frame.x_hidden : frame.x); }
  inline const int &getY() const
  { return ((hidden) ? frame.y_hidden : frame.y); }

    unsigned int width() const { return frame.width; }
    unsigned int height() const { return frame.height; }

  void addClient(Window);
  void removeClient(SlitClient *, Bool = True);
  void removeClient(Window, Bool = True);
  void reconfigure();
  void reposition();
  void shutdown();

  void buttonPressEvent(XButtonEvent *);
  void enterNotifyEvent(XCrossingEvent *);
  void leaveNotifyEvent(XCrossingEvent *);
  void configureRequestEvent(XConfigureRequestEvent *);

  virtual void timeout();

  enum { Vertical = 1, Horizontal };
  enum { TopLeft = 1, CenterLeft, BottomLeft, TopCenter, BottomCenter,
         TopRight, CenterRight, BottomRight };
};


#endif // __Slit_hh

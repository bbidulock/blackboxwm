// Slit.hh for Blackbox - an X11 Window manager
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

// forward declaration
class Slit;
class Slitmenu;

#include "Basemenu.hh"
#include "LinkedList.hh"


class Slitmenu : public Basemenu {
private: 
  class Directionmenu : public Basemenu {
  private:
    Slitmenu *slitmenu;
  
  protected:
    virtual void itemSelected(int, int);

  public:
    Directionmenu(Slitmenu *);
    
#ifdef    DEBUG
    virtual ~Directionmenu(void);
#endif // DEBUG
  }; 
  
  class Placementmenu : public Basemenu {
  private:
    Slitmenu *slitmenu;

  protected: 
    virtual void itemSelected(int, int);
  
  public:
    Placementmenu(Slitmenu *);

#ifdef    DEBUG
    virtual ~Placementmenu(void);
#endif // DEBUG
  };
  
  Directionmenu *directionmenu;
  Placementmenu *placementmenu;
  
  Slit *slit;
  
  friend Directionmenu;
  friend Placementmenu;


protected:
  virtual void itemSelected(int, int);


public:
  Slitmenu(Slit *);
  virtual ~Slitmenu(void);

  inline Basemenu *getDirectionmenu(void) { return directionmenu; }
  inline Basemenu *getPlacementmenu(void) { return placementmenu; }

  void reconfigure(void);
};


class Slit {
private:
  class SlitClient {
  public:
    Window window, client_window, icon_window;

    int x, y;
    unsigned int width, height;

#ifdef    DEBUG
    SlitClient(void);
    ~SlitClient(void);
#endif // DEBUG
  };
  
  Bool on_top;
  Display *display;
  
  Blackbox *blackbox;
  BScreen *screen;
  LinkedList<SlitClient> *clientList;
  Slitmenu *slitmenu;
  
  struct frame {
    Pixmap pixmap;
    Window window;
    
    int x, y;
    unsigned int width, height;
  } frame;

  friend Slitmenu;
  friend Slitmenu::Directionmenu;
  friend Slitmenu::Placementmenu;
  
  
protected:
  
  
public:
  Slit(BScreen *);
  ~Slit();

  inline const Bool &isOnTop(void) const { return on_top; }

  inline Slitmenu *getMenu() { return slitmenu; }

  inline const Window &getWindowID() const { return frame.window; }

  inline const int &getX(void) const { return frame.x; }
  inline const int &getY(void) const { return frame.y; }

  inline const unsigned int &getWidth(void) const { return frame.width; }
  inline const unsigned int &getHeight(void) const { return frame.height; }
  
  void addClient(Window);
  void removeClient(Window, Bool = True);
  void reconfigure(void);
  void reposition(void);

  void buttonPressEvent(XButtonEvent *);
  void configureRequestEvent(XConfigureRequestEvent *);

  enum { Vertical = 1, Horizontal };
  enum { TopLeft = 1, CenterLeft, BottomLeft, TopCenter, BottomCenter,
         TopRight, CenterRight, BottomRight };
};


#endif // __Slit_hh

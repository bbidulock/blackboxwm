// Toolbar.hh for Blackbox - an X11 Window manager
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
  
#ifndef   __Toolbar_hh
#define   __Toolbar_hh

#include <X11/Xlib.h>

// forward declaration
class Toolbar;

#include "Basemenu.hh"
#include "LinkedList.hh"
#include "Timer.hh"


class Toolbarmenu : public Basemenu {
private:
  class Placementmenu : public Basemenu {
  private:
    Toolbarmenu *toolbarmenu;

  protected:
    virtual void itemSelected(int, int);

  public:
    Placementmenu(Toolbarmenu *);
  };

  Toolbar *toolbar;
  Placementmenu *placementmenu;

  friend Placementmenu;


protected:
  virtual void itemSelected(int, int);

public:
  Toolbarmenu(Toolbar *);
  ~Toolbarmenu(void);

  inline Basemenu *getPlacementmenu(void) { return placementmenu; }

  void reconfigure(void);
};


class Toolbar : public TimeoutHandler {
private:
  Bool on_top, editing;
  Display *display;

  struct frame {
    Pixmap base, label, wlabel, clk, button, pbutton;
    Window window, workspace_label, window_label, clock, psbutton, nsbutton,
      pwbutton, nwbutton;

    int x, y, hour, minute, grab_x, grab_y;
    unsigned int width, height, window_label_w, window_label_h,
      workspace_label_w, workspace_label_h, clock_w, clock_h,
      button_w, button_h, bevel_w, label_h_2x;
  } frame;
  
  Blackbox *blackbox;
  BImageControl *image_ctrl;
  BScreen *screen;
  BTimer *timer;
  Toolbarmenu *toolbarmenu;
 
  char *new_workspace_name, *new_name_pos;

  friend Toolbarmenu;
  friend Toolbarmenu::Placementmenu;


protected:


public:
  Toolbar(BScreen *);
  virtual ~Toolbar(void);

  inline Toolbarmenu *getMenu(void) { return toolbarmenu; }
  
  inline const Bool &isEditing(void) const { return editing; }
  inline const Bool &isOnTop(void) const { return on_top; }

  inline const Window &getWindowID(void) const { return frame.window; }
  
  inline const unsigned int &getWidth(void) const { return frame.width; }
  inline const unsigned int &getHeight(void) const { return frame.height; }
  inline const int &getX(void) const { return frame.x; }
  inline const int &getY(void) const { return frame.y; }

  void buttonPressEvent(XButtonEvent *);
  void buttonReleaseEvent(XButtonEvent *);
  void exposeEvent(XExposeEvent *);
  void keyPressEvent(XKeyEvent *);

  void redrawWindowLabel(Bool = False);
  void redrawWorkspaceLabel(Bool = False);
  void redrawPrevWorkspaceButton(Bool = False, Bool = False);
  void redrawNextWorkspaceButton(Bool = False, Bool = False);
  void redrawPrevWindowButton(Bool = False, Bool = False);
  void redrawNextWindowButton(Bool = False, Bool = False);
  void edit(void);
  void reconfigure(void);
  
#ifdef    HAVE_STRFTIME
  void checkClock(Bool = False);
#else //  HAVE_STRFTIME
  void checkClock(Bool = False, Bool = False);
#endif // HAVE_STRFTIME

  virtual void timeout(void);

  enum { TopLeft = 1, BottomLeft, TopCenter,
         BottomCenter, TopRight, BottomRight };
};


#endif // __Toolbar_hh

// Icon.hh for Blackbox - an X11 Window manager
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
  
#ifndef   __Icon_hh
#define   __Icon_hh

class IconMenu;
class BlackboxIcon;

class Blackbox;
class BScreen;
class Toolbar;
class BlackboxWindow;

#include "Basemenu.hh"
#include "LinkedList.hh"


class Iconmenu : public Basemenu {
private:
  Blackbox *blackbox;
  BScreen *screen;
  LinkedList<BlackboxIcon> *iconList;


protected:
  virtual void itemSelected(int, int);


public:
  Iconmenu(BScreen *);
  virtual ~Iconmenu(void);

  int insert(BlackboxIcon *);
  int remove(BlackboxIcon *);
};


class BlackboxIcon {
private:
  Display *display;
  Window client;

  char *name;
  int icon_number;
  
  BlackboxWindow *window;
  BScreen *screen;


protected:


public:
  BlackboxIcon(BlackboxWindow *);
  ~BlackboxIcon(void);

  inline BlackboxWindow *getWindow(void) { return window; }

  inline char **getULabel(void) { return &name; }

  inline const int &getIconNumber(void) const { return icon_number; }

  inline void setIconNumber(int n) { icon_number = n; }

  void rereadLabel(void);
};


#endif // __Icon_hh

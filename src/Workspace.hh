// Workspace.hh for Blackbox - an X11 Window manager
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
  
#ifndef   __Workspace_hh
#define   __Workspace_hh

#include <X11/Xlib.h>

class BScreen;
class Clientmenu;
class Workspace;
class BlackboxWindow;

#include "LinkedList.hh"


class Workspace {
private:
  BScreen *screen;
  Clientmenu *clientmenu;

  LinkedList<BlackboxWindow> *windowList;

  char *name;
  int id, cascade_x, cascade_y;


protected:
  void placeWindow(BlackboxWindow *);


public:
  Workspace(BScreen *, int = 0);
  ~Workspace(void);

  inline BScreen *getScreen(void) { return screen; }

  inline Clientmenu *getMenu(void) { return clientmenu; }
 
  inline char *getName(void) { return name; }

  inline const int &getWorkspaceID(void) const { return id; }

  BlackboxWindow *getWindow(int);

  Bool isCurrent(void);

  const int addWindow(BlackboxWindow *, Bool = False);
  const int removeWindow(BlackboxWindow *);
  const int getCount(void);
  
  void showAll(void);
  void hideAll(void);
  void removeAll(void);
  void raiseWindow(BlackboxWindow *);
  void lowerWindow(BlackboxWindow *);
  // void setFocusWindow(int);
  void reconfigure();
  void update();
  void setCurrent(void);
  void setName(char *);
  void shutdown(void);
};


#endif // __Workspace_hh


//
// workspace.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// (See the included file COPYING / GPL-2.0)
//

#ifndef _blackbox_workspace_hh
#define _blackbox_workspace_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

class BlackboxSession;
class BlackboxWindow;
class Workspace;
class WorkspaceMenu;
class WorkspaceManager;
class WorkspaceManagerMenu;

#include "menu.hh"
#include "llist.hh"
#include "icon.hh"


class WorkspaceMenu : public BlackboxMenu {
private:
  Workspace *workspace;


protected:
  virtual void titlePressed(int);
  virtual void titleReleased(int);
  virtual void itemPressed(int, int);
  virtual void itemReleased(int, int);


public:
  WorkspaceMenu(Workspace *, BlackboxSession *s);

  void showMenu();
  void hideMenu();
  void moveMenu(int, int);
  void updateMenu();

  int insert(char **);
  int remove(int);
};


class WorkspaceManagerMenu : public BlackboxMenu {
private:
  WorkspaceManager *ws_manager;
  

protected:
  virtual void titlePressed(int) { }
  virtual void titleReleased(int) { }
  virtual void itemPressed(int, int);
  virtual void itemReleased(int, int);


public:
  WorkspaceManagerMenu(WorkspaceManager *, BlackboxSession *);

  void showMenu();
  void hideMenu();
  void moveMenu(int, int);
  void updateMenu();

  int insert(char *);
  int insert(char *, WorkspaceMenu *);
  int remove(int);
};


class Workspace {
private:
  WorkspaceManager *ws_manager;
  llist<BlackboxWindow> *workspace_list;
  WorkspaceMenu *workspace_menu;

  char *workspace_name;
  int workspace_id;


protected:


public:
  Workspace(WorkspaceManager *, int = 0);
  ~Workspace(void);

  const int addWindow(BlackboxWindow *);
  const int removeWindow(BlackboxWindow *);
  int showAll(void);
  int hideAll(void);
  int removeAll(void);
  void Dissociate(void);
  void showMenu(void);
  void hideMenu(void);
  void moveMenu(int, int);
  void updateMenu(void);
  
  int workspaceID(void)
    { return workspace_id; }
  char *name(void)
    { return workspace_name; }
  WorkspaceMenu *menu(void)
    { return workspace_menu; }
  int menuVisible(void);
};


class WorkspaceManager {
private:
  Bool sub;
  Display *display;
  GC buttonGC;

  struct frame {
    Pixmap button, pbutton;
    Window base,
      icon,
      window,
      workspace_button;

    unsigned int frame_w, frame_h,
      button_w, button_h;
    char *title;
  } frame;

  llist<Workspace> *workspaces_list;
  llist<BlackboxIcon> *ilist;
  WorkspaceManagerMenu *workspaces_menu;
  BlackboxSession *session;
  Workspace *current;
  
  friend WorkspaceManagerMenu;


protected:


public:
  WorkspaceManager(BlackboxSession *, int = 1);
  ~WorkspaceManager(void);

  int addWorkspace(void);
  int removeWorkspaceID(int);
  void changeWorkspaceID(int);
  Workspace *workspace(int);
  void DissociateAll(void);
  void addIcon(BlackboxIcon *i) { ilist->insert(i); arrangeIcons(); }
  void removeIcon(BlackboxIcon *i) { ilist->remove(i); arrangeIcons(); }
  void arrangeIcons(void);

  void buttonPressEvent(XButtonEvent *);
  void buttonReleaseEvent(XButtonEvent *);
  void exposeEvent(XExposeEvent *);
  Window windowID(void)
    { return frame.window; }
  Window iconWindowID(void)
    { return frame.icon; }
  Workspace *currentWorkspace(void)
    { return current; }
  BlackboxSession *Session(void)
    { return session; }
  int count(void)
    { return workspaces_list->count(); }
  int currentWorkspaceID(void)
    { return current->workspaceID(); }
};


#endif

// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Screen.hh for Blackbox - an X11 Window manager
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

#ifndef   __Screen_hh
#define   __Screen_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xresource.h>

#ifdef    TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else // !TIME_WITH_SYS_TIME
#  ifdef    HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else // !HAVE_SYS_TIME_H
#    include <time.h>
#  endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME
}

#include <list>
#include <vector>
typedef std::vector<Window> WindowStack;

#include "Color.hh"
#include "Image.hh"
#include "Texture.hh"
#include "Configmenu.hh"
#include "Iconmenu.hh"
#include "Netwm.hh"
#include "Workspacemenu.hh"
#include "blackbox.hh"

// forward declaration
class Rootmenu;
class Slit;
class Workspace;

typedef std::list<BlackboxWindow*> BlackboxWindowList;
typedef std::list<Window> WindowList;

enum PlacementPolicy { RowSmartPlacement = 400, ColSmartPlacement,
                       CascadePlacement };
enum PlacementDirection { LeftRight = 403, RightLeft, TopBottom, BottomTop };

struct ScreenResource: public bt::NoCopy {
public:
  struct WindowStyle {
    bt::Color l_text_focus, l_text_unfocus, b_pic_focus,
      b_pic_unfocus;
    bt::Texture f_focus, f_unfocus, t_focus, t_unfocus, l_focus, l_unfocus,
      h_focus, h_unfocus, b_focus, b_unfocus, b_pressed, g_focus, g_unfocus;
    bt::Font font;
    bt::Alignment alignment;
    unsigned int handle_height, grip_width, frame_width, bevel_width,
      label_height, title_height, button_width;
  };

  struct ToolbarStyle {
    bt::Color l_text, w_text, c_text, b_pic;
    bt::Texture toolbar, label, window, button, pressed, clock;
    bt::Font font;
    bt::Alignment alignment;
  };

  ScreenResource(void) {}
  ~ScreenResource(void) {}

  void loadStyle(BScreen* screen, const std::string& style);
  void loadRCFile(unsigned int screen, const std::string& rc);

  // access functions
  WindowStyle* windowStyle(void)   { return &wstyle; }
  ToolbarStyle* toolbarStyle(void) { return &tstyle; }
  bt::Color* borderColor(void)     { return &border_color; }

  unsigned int bevelWidth(void) const         { return bevel_width;       }
  unsigned int borderWidth(void) const        { return border_width;      }
  unsigned int numberOfWorkspaces(void) const { return workspace_count;   }
  bool allowScrollLock(void) const            { return allow_scroll_lock; }
  const std::string& rootCommand(void) const  { return root_command;      }
  bt::DitherMode ditherMode(void) const       { return dither_mode;       }
  const std::string& workspaceName(unsigned int i) const
                                              { return workspaces[i];     }

  bool isSloppyFocus(void) const        { return wconfig.sloppy_focus;  }
  bool doAutoRaise(void) const          { return wconfig.auto_raise;    }
  bool doClickRaise(void) const         { return wconfig.click_raise;   }
  bool doOpaqueMove(void) const         { return wconfig.opaque_move;   }
  bool doFullMax(void) const            { return wconfig.full_max;      }
  bool placementIgnoresShaded(void) const
                                        { return wconfig.ignore_shaded; }
  bool doFocusNew(void) const           { return wconfig.focus_new;     }
  bool doFocusLast(void) const          { return wconfig.focus_last;    }
  int placementPolicy(void) const       { return wconfig.placement_policy; }
  int edgeSnapThreshold(void) const     { return wconfig.edge_snap_threshold; }
  int rowPlacementDirection(void) const { return wconfig.row_direction; }
  int colPlacementDirection(void) const { return wconfig.col_direction; }

  bool isToolbarOnTop(void) const     { return tconfig.on_top;        }
  bool doToolbarAutoHide(void) const  { return tconfig.auto_hide;     }
  int toolbarPlacement(void) const    { return tconfig.placement;     }
  int toolbarWidthPercent(void) const { return tconfig.width_percent; }
  const char* strftimeFormat(void) const
                                    { return tconfig.strftime_format.c_str(); }

  bool isSlitOnTop(void) const    { return sconfig.on_top;    }
  bool doSlitAutoHide(void) const { return sconfig.auto_hide; }
  int slitPlacement(void) const   { return sconfig.placement; }
  int slitDirection(void) const   { return sconfig.direction; }

  // store functions
  void saveWorkspaces(unsigned int w)      { workspace_count = w;   }
  void saveAllowScrollLock(bool a)         { allow_scroll_lock = a; }
  void saveBorderColor(const bt::Color& c) { border_color = c;      }
  void saveBorderWidth(unsigned int i)     { border_width = i;      }
  void saveBevelWidth(unsigned int i)      { bevel_width = i;       }
  void saveWorkspaceName(unsigned int w, const std::string& name)
                                           { workspaces[w] = name;  }

  void saveSlitPlacement(int i) { sconfig.placement = i; }
  void saveSlitDirection(int i) { sconfig.direction = i; }
  void saveSlitOnTop(bool b)    { sconfig.on_top = b;    }
  void saveSlitAutoHide(bool b) { sconfig.auto_hide = b; }

  void saveToolbarOnTop(bool b)       { tconfig.on_top = b;          }
  void saveToolbarAutoHide(bool b)    { tconfig.auto_hide = b;       }
  void saveToolbarWidthPercent(int i) { tconfig.width_percent = i;   }
  void saveToolbarPlacement(int i)    { tconfig.placement = i;       }
  void saveStrftimeFormat(const std::string& f)
                                      { tconfig.strftime_format = f; }

  void saveOpaqueMove(bool o)             { wconfig.opaque_move = o;         }
  void saveFullMax(bool f)                { wconfig.full_max = f;            }
  void saveFocusNew(bool f)               { wconfig.focus_new = f;           }
  void saveFocusLast(bool f)              { wconfig.focus_last = f;          }
  void saveSloppyFocus(bool s)            { wconfig.sloppy_focus = s;        }
  void saveAutoRaise(bool a)              { wconfig.auto_raise = a;          }
  void saveClickRaise(bool c)             { wconfig.click_raise = c;         }
  void savePlacementPolicy(PlacementPolicy p)
                                          { wconfig.placement_policy = p;    }
  void saveRowPlacementDirection(PlacementDirection d)
                                          { wconfig.row_direction = d;       }
  void saveColPlacementDirection(PlacementDirection d)
                                          { wconfig.col_direction = d;       }
  void savePlacementIgnoresShaded(bool f) { wconfig.ignore_shaded = f;       }
  void saveEdgeSnapThreshold(int t)       { wconfig.edge_snap_threshold = t; }

private:
  struct WindowConfig {
    bool sloppy_focus, auto_raise, opaque_move, full_max, ignore_shaded,
      focus_new, focus_last, click_raise;
    int edge_snap_threshold;
    PlacementPolicy placement_policy;
    PlacementDirection row_direction, col_direction;
  };

  struct ToolbarConfig {
    bool on_top, auto_hide;
    int placement, width_percent;
    std::string strftime_format;
  };

  struct SlitConfig {
    bool on_top, auto_hide;
    int placement, direction;
  };

  WindowStyle wstyle;
  WindowConfig wconfig;
  ToolbarStyle tstyle;
  ToolbarConfig tconfig;
  SlitConfig sconfig;

  bool allow_scroll_lock;
  unsigned int workspace_count, bevel_width, border_width;
  bt::Color border_color;
  bt::DitherMode dither_mode;
  std::string root_command;
  std::vector<std::string> workspaces;
};

class BScreen : public bt::NoCopy, public bt::EventHandler {
private:
  bool root_colormap_installed, managed, geom_visible;
  GC opGC;
  Pixmap geom_pixmap;
  Window geom_window;

  const bt::ScreenInfo& screen_info;
  Blackbox *blackbox;
  Configmenu *configmenu;
  Iconmenu *iconmenu;
  Rootmenu *rootmenu;

  BlackboxWindowList iconList, windowList;

  Slit *slit;
  Toolbar *toolbar;
  Workspace *current_workspace;
  unsigned int current_workspace_id;
  Workspacemenu *workspacemenu;

  unsigned int geom_w, geom_h;
  unsigned long event_mask;

  bt::Rect usableArea;

  typedef std::list<bt::Netwm::Strut*> StrutList;
  StrutList strutList;
  typedef std::vector<Workspace*> WorkspaceList;
  WorkspaceList workspacesList;

  ScreenResource _resource;

  bool parseMenuFile(FILE *file, Rootmenu *menu);

  void InitMenu(void);
  void LoadStyle(void);

  void manageWindow(Window w);
  void unmanageWindow(BlackboxWindow *w, bool remap);

  void updateAvailableArea(void);
  void updateWorkareaHint(void) const;

public:
  enum { RoundBullet = 1, TriangleBullet, SquareBullet, NoBullet };
  enum { Restart = 1, RestartOther, Exit, Shutdown, Execute, Reconfigure,
         WindowShade, WindowIconify, WindowMaximize, WindowClose, WindowRaise,
         WindowLower, WindowKill, SetStyle };
  enum FocusModel { SloppyFocus, ClickToFocus };

  BScreen(Blackbox *bb, unsigned int scrn);
  ~BScreen(void);

  // information about the screen
  const bt::ScreenInfo &screenInfo(void) const { return screen_info; }
  unsigned int screenNumber(void) const { return screen_info.screenNumber(); }

  ScreenResource& resource(void) { return _resource; }

  bool isScreenManaged(void) const { return managed; }
  bool isRootColormapInstalled(void) const { return root_colormap_installed; }

  const GC &getOpGC(void) const { return opGC; }
  Blackbox *getBlackbox(void) { return blackbox; }

  Rootmenu *getRootmenu(void) { return rootmenu; }

  Slit *getSlit(void) { return slit; }

  Toolbar *getToolbar(void) { return toolbar; }

  Workspace *getWorkspace(unsigned int index) const;
  const std::string& getWorkspaceName(unsigned int index) const;

  Workspace *getCurrentWorkspace(void) { return current_workspace; }

  Workspacemenu *getWorkspacemenu(void) { return workspacemenu; }

  const Time& doubleClickInterval(void) const
  { return blackbox->doubleClickInterval(); }

  unsigned int getCurrentWorkspaceID(void) const
  { return current_workspace_id; }
  unsigned int getWorkspaceCount(void) const
  { return workspacesList.size(); }
  unsigned int getIconCount(void) const { return iconList.size(); }

  BlackboxWindow* getWindow(unsigned int workspace, unsigned int id);

  void setRootColormapInstalled(bool r) { root_colormap_installed = r; }

  BlackboxWindow *getIcon(unsigned int index);

  const bt::Rect& availableArea(void);
  void addStrut(bt::Netwm::Strut *strut);
  void removeStrut(bt::Netwm::Strut *strut);
  void updateStrut(void);

  void updateClientListHint(void) const;
  void updateClientListStackingHint(void) const;
  void updateDesktopNamesHint(void) const;

  unsigned int addWorkspace(void);
  unsigned int removeLastWorkspace(void);
  void setWorkspaceName(unsigned int workspace, const std::string& name);
  const std::string getNameOfWorkspace(unsigned int id);
  void changeWorkspaceID(unsigned int id);
  void getDesktopNames(void);

  void iconifyWindow(BlackboxWindow *w);
  void removeIcon(BlackboxWindow *w);
  void addWindow(Window w);
  void releaseWindow(BlackboxWindow *w, bool remap);
  void raiseWindow(BlackboxWindow *w);
  void lowerWindow(BlackboxWindow *w);
  void reassociateWindow(BlackboxWindow *w, unsigned int wkspc_id);
  void propagateWindowName(const BlackboxWindow *w);

  void raiseWindows(const bt::Netwm::WindowList* const workspace_stack);

  void nextFocus(void) const;
  void prevFocus(void) const;
  void raiseFocus(void) const;
  void reconfigure(void);
  void toggleFocusModel(FocusModel model);
  void rereadMenu(void);
  void shutdown(void);
  void showPosition(int x, int y);
  void showGeometry(unsigned int gx, unsigned int gy);
  void hideGeometry(void);

  void clientMessageEvent(const XClientMessageEvent * const event);
  void buttonPressEvent(const XButtonEvent * const event);
  void configureRequestEvent(const XConfigureRequestEvent * const event);
};


#endif // __Screen_hh

// Screen.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
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

#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Icon.hh"
#include "Image.hh"
#include "Screen.hh"

#ifdef    SLIT
#include "Slit.hh"
#endif // SLIT

#include "Rootmenu.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#ifndef   MAXPATHLEN
#define   MAXPATHLEN 255
#endif // MAXPATHLEN


static Bool running = True;

static int anotherWMRunning(Display *display, XErrorEvent *) {
  fprintf(stderr,
	  "BScreen::BScreen: an error occured while querying the X server.\n"
	  "  another window manager already running on display %s.\n",
          DisplayString(display));
  
  running = False;
  
  return(-1);
}


BScreen::BScreen(Blackbox *bb, int scrn) : ScreenInfo(bb, scrn) {
  blackbox = bb;

  event_mask = ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
    SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
    ButtonPressMask | ButtonReleaseMask;
  
  XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(getDisplay()->getDisplay(), getRootWindow(), event_mask);
  XSync(getDisplay()->getDisplay(), False);
  XSetErrorHandler((XErrorHandler) old);

  managed = running;
  if (! managed) return;
  
  rootmenu = 0;
  resource.stylerc = 0;
  resource.font.menu = resource.font.title = (XFontStruct *) 0;
 
#ifdef    HAVE_STRFTIME
  resource.strftime_format = 0;
#endif // HAVE_STRFTIME
  
  XDefineCursor(getDisplay()->getDisplay(), getRootWindow(),
                blackbox->getSessionCursor());

  workspaceNames = new LinkedList<char>;
  workspacesList = new LinkedList<Workspace>;
  
  rootmenuList = new LinkedList<Rootmenu>;
  
#ifdef    KDE  
  kwm_module_list = new LinkedList<Window>;
  kwm_window_list = new LinkedList<Window>;
  
  unsigned long data = 1;
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(),
                  blackbox->getKWMRunningAtom(),
		  blackbox->getKWMRunningAtom(), 32, PropModeAppend,
		  (unsigned char *) &data, 1);
  
  Atom a;
  a = XInternAtom(getDisplay()->getDisplay(), "KWM_STRING_MAXIMIZE", False);
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(), a, XA_STRING,
                  8, PropModeReplace, (unsigned char *) "&Maximize", 10);
  
  a = XInternAtom(getDisplay()->getDisplay(), "KWM_STRING_UNMAXIMIZE", False);
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(), a, XA_STRING,
                  8, PropModeReplace, (unsigned char *) "&Un Maximize", 12);
  
  a = XInternAtom(getDisplay()->getDisplay(), "KWM_STRING_ICONIFY", False);
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(), a, XA_STRING,
                  8, PropModeReplace, (unsigned char *) "&Iconify", 9);
  
  a = XInternAtom(getDisplay()->getDisplay(), "KWM_STRING_UNICONIFY", False);
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(), a, XA_STRING,
                  8, PropModeReplace, (unsigned char *) "&De Iconify", 9);
  
  a = XInternAtom(getDisplay()->getDisplay(),"KWM_STRING_STICKY",False);
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(), a, XA_STRING,
                  8, PropModeReplace, (unsigned char *) "&Stick", 7);
  
  a = XInternAtom(getDisplay()->getDisplay(), "KWM_STRING_UNSTICKY", False);
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(), a, XA_STRING,
                  8, PropModeReplace, (unsigned char *) "&Un Stick", 9);
  
  a = XInternAtom(getDisplay()->getDisplay(), "KWM_STRING_MOVE", False);
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(), a, XA_STRING,
                  8, PropModeReplace, (unsigned char *) "&Move", 6);
  
  a = XInternAtom(getDisplay()->getDisplay(), "KWM_STRING_RESIZE", False);
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(), a, XA_STRING,
                  8, PropModeReplace, (unsigned char *) "&Resize", 8);
  
  a = XInternAtom(getDisplay()->getDisplay(), "KWM_STRING_CLOSE", False);
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(), a, XA_STRING,
                  8, PropModeReplace, (unsigned char *) "&Close", 7);
  
  a = XInternAtom(getDisplay()->getDisplay(), "KWM_STRING_ONTOCURRENTDESKTOP",
                  False);
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(), a, XA_STRING,
                  8, PropModeReplace,
                  (unsigned char *) "R&eassociate with current...", 7);
#endif // KDE
  
  image_control =
    new BImageControl(blackbox, this, blackbox->hasImageDither(),
                      blackbox->getColorsPerChannel());
  image_control->installRootColormap();
  root_colormap_installed = True;
  
  blackbox->load_rc(this);
  
  LoadStyle();
  
  XGCValues gcv;
  gcv.foreground = WhitePixel(getDisplay()->getDisplay(), getScreenNumber());
  gcv.function = GXxor;
  gcv.subwindow_mode = IncludeInferiors;
  opGC = XCreateGC(getDisplay()->getDisplay(), getRootWindow(),
                   GCForeground | GCFunction | GCSubwindowMode, &gcv);

  gcv.foreground = getWResource()->decoration.unfocusTextColor.getPixel();
  gcv.font = getTitleFont()->fid;
  wunfocusGC = XCreateGC(getDisplay()->getDisplay(), getRootWindow(),
                         GCForeground | GCFont, &gcv);
  
  gcv.foreground = getWResource()->decoration.focusTextColor.getPixel();
  gcv.font = getTitleFont()->fid;
  wfocusGC = XCreateGC(getDisplay()->getDisplay(), getRootWindow(),
                       GCForeground | GCFont, &gcv);

  gcv.foreground = getMResource()->title.textColor.getPixel();
  gcv.font = getTitleFont()->fid;
  mtitleGC = XCreateGC(getDisplay()->getDisplay(), getRootWindow(),
                       GCForeground | GCFont, &gcv);

  gcv.foreground = getMResource()->frame.textColor.getPixel();
  gcv.font = getMenuFont()->fid;
  mframeGC = XCreateGC(getDisplay()->getDisplay(), getRootWindow(),
                       GCForeground | GCFont, &gcv);

  gcv.foreground = getMResource()->frame.hiTextColor.getPixel();
  mhiGC = XCreateGC(getDisplay()->getDisplay(), getRootWindow(),
                    GCForeground | GCFont, &gcv);

  gcv.foreground = getMResource()->frame.hiColor.getPixel();
  gcv.arc_mode = ArcChord;
  gcv.fill_style = FillSolid;
  mhbgGC = XCreateGC(getDisplay()->getDisplay(), getRootWindow(),
                     GCForeground | GCFillStyle | GCArcMode, &gcv);

  geom_h = getTitleFont()->ascent + getTitleFont()->descent +
    (resource.bevel_width * 2);
  geom_w = (resource.bevel_width * 2) +
    XTextWidth(getTitleFont(), "0: 0000 x 0: 0000",
                        strlen("0: 0000 x 0: 0000"));

  geom_pixmap =
    image_control->renderImage(geom_w, geom_h,
                               &(getWResource()->decoration.focusTexture));

  XGrabKey(getDisplay()->getDisplay(),
           XKeysymToKeycode(getDisplay()->getDisplay(), XK_Tab),
           blackbox->getWindowCycleMask(), getRootWindow(), True,
           GrabModeAsync, GrabModeAsync);
  XGrabKey(getDisplay()->getDisplay(),
           XKeysymToKeycode(getDisplay()->getDisplay(), XK_Tab),
           blackbox->getWindowCycleMask() | ShiftMask, getRootWindow(), True,
           GrabModeAsync, GrabModeAsync);
  XGrabKey(getDisplay()->getDisplay(),
           XKeysymToKeycode(getDisplay()->getDisplay(), XK_Left),
           blackbox->getWorkspaceChangeMask(), getRootWindow(), True,
           GrabModeAsync, GrabModeAsync);
  XGrabKey(getDisplay()->getDisplay(),
           XKeysymToKeycode(getDisplay()->getDisplay(), XK_Right),
           blackbox->getWorkspaceChangeMask(), getRootWindow(), True,
           GrabModeAsync, GrabModeAsync);

  XSetWindowAttributes attrib;
  unsigned long mask = CWBackPixmap | CWBorderPixel | CWSaveUnder;
  attrib.background_pixmap = geom_pixmap;
  attrib.border_pixel = getBorderColor()->getPixel();
  attrib.save_under = True;

  geom_window =
    XCreateWindow(getDisplay()->getDisplay(), getRootWindow(),
                  0, 0, geom_w, geom_h, resource.border_width, getDepth(),
                  InputOutput, getVisual(), mask, &attrib);
  geom_visible = False;
  
  workspacemenu = new Workspacemenu(blackbox, this);
  iconmenu = new Iconmenu(blackbox, this);
  
  Workspace *wkspc = (Workspace *) 0;
  if (resource.workspaces != 0) {
    for (int i = 0; i < resource.workspaces; ++i) {
      wkspc = new Workspace(this, workspacesList->count());
      workspacesList->insert(wkspc);
      workspacemenu->insert(wkspc->getName(), wkspc->getMenu());
    }
  } else {
    wkspc = new Workspace(this, workspacesList->count());
    workspacesList->insert(wkspc);
    workspacemenu->insert(wkspc->getName(), wkspc->getMenu());
  }
  
#ifdef    KDE
  data = (unsigned long) workspacesList->count();

  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(),
		  blackbox->getKWMNumberOfDesktopsAtom(),
		  blackbox->getKWMNumberOfDesktopsAtom(), 32,
		  PropModeReplace, (unsigned char *) &data, 1);
  
  sendToKWMModules(blackbox->getKWMModuleDesktopNumberChangeAtom(),
		   (XID) data);
#endif // KDE
    
  workspacemenu->insert("Icons", iconmenu);
  workspacemenu->update();
  
  current_workspace = workspacesList->first();
  workspacemenu->setHighlight(2);
  
  toolbar = new Toolbar(blackbox, this);

#ifdef    SLIT
  slit = new Slit(blackbox, this);
#endif // SLIT
  
  InitMenu();
  
  raiseWindows(0, 0);
  rootmenu->update();

  changeWorkspaceID(0);

  int i;
  unsigned int nchild;
  Window r, p, *children;
  XQueryTree(getDisplay()->getDisplay(), getRootWindow(), &r, &p, &children,
             &nchild);

  // preen the window list of all icon windows... for better dockapp support
  for (i = 0; i < (int) nchild; i++) {
    if (children[i] == None) continue;

    XWMHints *wmhints = XGetWMHints(getDisplay()->getDisplay(), children[i]);

    if (wmhints) {
      if ((wmhints->flags & IconWindowHint) &&
	  (wmhints->icon_window != children[i])) {
        for (int j = 0; j < (int) nchild; j++) {
          if (children[j] == wmhints->icon_window) {
            children[j] = None;
	    
            break;
          }
	}
      }
      
      XFree(wmhints);
    }
  }
 
  // manage shown windows
  for (i = 0; i < (int) nchild; ++i) {
    if (children[i] == None || (! blackbox->validateWindow(children[i])))
      continue;
    
    XWindowAttributes attrib;
    if (XGetWindowAttributes(getDisplay()->getDisplay(), children[i],
                             &attrib)) {
      if (attrib.override_redirect) continue;

#ifdef    KDE
      addKWMModule(children[i]);
#endif // KDE

#ifdef    SLIT
      XWMHints *wmhints = XGetWMHints(getDisplay()->getDisplay(), children[i]);
	
      if (wmhints &&
          (wmhints->flags & StateHint) &&
	  (wmhints->initial_state == WithdrawnState) &&
          (! blackbox->searchSlit(children[i]))) {
	slit->addClient(children[i]);

        XFree(wmhints);
      } else
#endif // SLIT

      if (attrib.map_state != IsUnmapped) {
        BlackboxWindow *win = new BlackboxWindow(blackbox, this,
						 children[i]);
	    
        XMapRequestEvent mre;
	mre.window = children[i];
	win->mapRequestEvent(&mre);
      }
    }
  }

  XFree(children);
  XSync(getDisplay()->getDisplay(), False);
}


BScreen::~BScreen(void) {
  if (! managed) return;

  if (geom_pixmap != None)
    image_control->removeImage(geom_pixmap);    

  if (geom_window != None)
    XDestroyWindow(getDisplay()->getDisplay(), geom_window);

  removeWorkspaceNames();

  while (workspacesList->count())
    delete workspacesList->remove(0);
  
  while (rootmenuList->count())
    rootmenuList->remove(0);
  
#ifdef    HAVE_STRFTIME
  if (resource.strftime_format) delete [] resource.strftime_format;
#endif // HAVE_STRFTIME

  delete rootmenu;
  delete workspacemenu;
  delete iconmenu;

#ifdef    SLIT
  delete slit;
#endif // SLIT

  delete toolbar;
  delete image_control;

  delete workspacesList;
  delete workspaceNames;

  delete rootmenuList;

#ifdef    KDE
  delete kwm_module_list;
  delete kwm_window_list;
#endif // KDE

  if (resource.font.title) {
    XFreeFont(getDisplay()->getDisplay(), resource.font.title);
    resource.font.title = 0;
  }
  
  if (resource.font.menu) {
    XFreeFont(getDisplay()->getDisplay(), resource.font.menu);
    resource.font.menu = 0;
  }

  XFreeGC(getDisplay()->getDisplay(), opGC);
  XFreeGC(getDisplay()->getDisplay(), wfocusGC);
  XFreeGC(getDisplay()->getDisplay(), wunfocusGC);
  XFreeGC(getDisplay()->getDisplay(), mtitleGC);
  XFreeGC(getDisplay()->getDisplay(), mframeGC);
  XFreeGC(getDisplay()->getDisplay(), mhiGC);
  XFreeGC(getDisplay()->getDisplay(), mhbgGC);
}

void BScreen::readDatabaseTexture(char *rname, char *rclass,
				  BTexture *texture) {
  XrmValue value;
  char *value_type;
  
  texture->setTexture(0);
  
  if (XrmGetResource(resource.stylerc, rname, rclass, &value_type,
		     &value))
    image_control->parseTexture(texture, value.addr);
  
  if (texture->getTexture() & BImage_Solid) {
    int clen = strlen(rclass) + 8, nlen = strlen(rname) + 8;
    char *colorclass = new char[clen], *colorname = new char[nlen];
    
    sprintf(colorclass, "%s.Color", rclass);
    sprintf(colorname,  "%s.color", rname);
    
    readDatabaseColor(colorname, colorclass, texture->getColor());
    
    delete [] colorclass;
    delete [] colorname;
    
    if ((! texture->getColor()->isAllocated()) ||
	(texture->getTexture() & BImage_Flat))
      return;
    
    XColor xcol;
    
    xcol.red = (unsigned int) (texture->getColor()->getRed() +
			       (texture->getColor()->getRed() >> 1));
    if (xcol.red >= 0xff) xcol.red = 0xffff;
    else xcol.red *= 0xff;
    xcol.green = (unsigned int) (texture->getColor()->getGreen() +
				 (texture->getColor()->getGreen() >> 1));
    if (xcol.green >= 0xff) xcol.green = 0xffff;
    else xcol.green *= 0xff;
    xcol.blue = (unsigned int) (texture->getColor()->getBlue() +
				(texture->getColor()->getBlue() >> 1));
    if (xcol.blue >= 0xff) xcol.blue = 0xffff;
    else xcol.blue *= 0xff;
    
    if (! XAllocColor(getDisplay()->getDisplay(), image_control->getColormap(),
                      &xcol))
      xcol.pixel = 0;
    
    texture->getHiColor()->setPixel(xcol.pixel);
    
    xcol.red =
      (unsigned int) ((texture->getColor()->getRed() >> 2) +
		      (texture->getColor()->getRed() >> 1)) * 0xff;
    xcol.green =
      (unsigned int) ((texture->getColor()->getGreen() >> 2) +
		      (texture->getColor()->getGreen() >> 1)) * 0xff;
    xcol.blue =
      (unsigned int) ((texture->getColor()->getBlue() >> 2) +
		      (texture->getColor()->getBlue() >> 1)) * 0xff;
    
    if (! XAllocColor(getDisplay()->getDisplay(), image_control->getColormap(),
                      &xcol))
      xcol.pixel = 0;
    
    texture->getLoColor()->setPixel(xcol.pixel);
  } else if (texture->getTexture() & BImage_Gradient) {
    int clen = strlen(rclass) + 10, nlen = strlen(rname) + 10;
    char *colorclass = new char[clen], *colorname = new char[nlen],
      *colortoclass = new char[clen], *colortoname = new char[nlen];
    
    sprintf(colorclass, "%s.Color", rclass);
    sprintf(colorname,  "%s.color", rname);
    
    sprintf(colortoclass, "%s.ColorTo", rclass);
    sprintf(colortoname,  "%s.colorTo", rname);
    
    readDatabaseColor(colorname, colorclass, texture->getColor());
    readDatabaseColor(colortoname, colortoclass, texture->getColorTo());
    
    delete [] colorclass;
    delete [] colorname;
    delete [] colortoclass;
    delete [] colortoname;
  }
}


void BScreen::readDatabaseColor(char *rname, char *rclass, BColor *color) {
  XrmValue value;
  char *value_type;
  
  if (XrmGetResource(resource.stylerc, rname, rclass, &value_type,
		     &value))
    image_control->parseColor(color, value.addr);
  else
    // parsing with no color string just deallocates the color, if it has
    // been previously allocated
    image_control->parseColor(color);
}


void BScreen::reconfigure(void) {
  LoadStyle();

  XGCValues gcv;
  gcv.foreground = image_control->getColor("white");
  gcv.font = getTitleFont()->fid;
  XChangeGC(getDisplay()->getDisplay(), opGC, GCForeground | GCFont, &gcv);

  gcv.foreground = getWResource()->decoration.unfocusTextColor.getPixel();
  gcv.font = getTitleFont()->fid;
  XChangeGC(getDisplay()->getDisplay(), wunfocusGC,
            GCForeground | GCBackground | GCFont, &gcv);

  gcv.foreground = getWResource()->decoration.focusTextColor.getPixel();
  gcv.font = getTitleFont()->fid;
  XChangeGC(getDisplay()->getDisplay(), wfocusGC,
            GCForeground | GCBackground | GCFont, &gcv);

  gcv.foreground = getMResource()->title.textColor.getPixel();
  gcv.font = getTitleFont()->fid;
  XChangeGC(getDisplay()->getDisplay(), mtitleGC, GCForeground | GCFont, &gcv);
  
  gcv.foreground = getMResource()->frame.textColor.getPixel();
  gcv.font = getMenuFont()->fid;
  XChangeGC(getDisplay()->getDisplay(), mframeGC, GCForeground | GCFont, &gcv);

  gcv.foreground = getMResource()->frame.hiTextColor.getPixel();
  XChangeGC(getDisplay()->getDisplay(), mhiGC,
            GCForeground | GCBackground | GCFont, &gcv);

  gcv.foreground = getMResource()->frame.hiColor.getPixel();
  gcv.arc_mode = ArcChord;
  gcv.fill_style = FillSolid;
  XChangeGC(getDisplay()->getDisplay(), mhbgGC,
            GCForeground | GCFillStyle | GCArcMode, &gcv);

  geom_h = getTitleFont()->ascent + getTitleFont()->descent +
    (resource.bevel_width * 2);
  geom_w = (resource.bevel_width * 2) +
    XTextWidth(getTitleFont(), "0: 0000 x 0: 0000",
                        strlen("0: 0000 x 0: 0000"));

  Pixmap tmp = geom_pixmap;
  geom_pixmap =
    image_control->renderImage(geom_w, geom_h,
                               &(getWResource()->decoration.focusTexture));
  if (tmp) image_control->removeImage(tmp);

  XSetWindowBackgroundPixmap(getDisplay()->getDisplay(), geom_window,
                             geom_pixmap);
  XSetWindowBorderWidth(getDisplay()->getDisplay(), geom_window,
                        resource.border_width);
  XSetWindowBorder(getDisplay()->getDisplay(), geom_window,
                   resource.border_color.getPixel());

  workspacemenu->reconfigure();
  iconmenu->reconfigure();
  
  InitMenu();
  rootmenu->reconfigure();

  toolbar->reconfigure();

#ifdef    SLIT
  slit->reconfigure();
#endif // SLIT

  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it++)
    it.current()->reconfigure();
}


void BScreen::rereadMenu(void) {
  InitMenu();

  rootmenu->reconfigure();
}


void BScreen::removeWorkspaceNames(void) {
  while (workspaceNames->count())
    delete [] workspaceNames->remove(0);
}


void BScreen::LoadStyle(void) {
  resource.stylerc = XrmGetFileDatabase(blackbox->getStyleFilename());
  if (! resource.stylerc)
    resource.stylerc = XrmGetFileDatabase(DEFAULTSTYLE);

  XrmValue value;
  char *value_type;

  // load window config
  readDatabaseTexture("window.focus", "Window.Focus",
		      &(resource.wres.decoration.focusTexture));
  readDatabaseTexture("window.unfocus", "Window.Unfocus",
		      &(resource.wres.decoration.unfocusTexture));
  readDatabaseTexture("window.frame", "Window.Frame",
		      &(resource.wres.frame.texture));
  readDatabaseTexture("window.focus.button", "Window.Focus.Button",
		      &(resource.wres.button.focusTexture));
  readDatabaseTexture("window.unfocus.button", "Window.Unfocus.Button",
		      &(resource.wres.button.unfocusTexture));
  readDatabaseTexture("window.button.pressed", "Window.Button.Pressed",
		      &(resource.wres.button.pressedTexture));
  readDatabaseColor("window.focus.textColor", "Window.Focus.TextColor",
		    &(resource.wres.decoration.focusTextColor));
  readDatabaseColor("window.unfocus.textColor", "Window.Unfocus.TextColor",
		    &(resource.wres.decoration.unfocusTextColor));
  
  // load menu configuration
  readDatabaseTexture("menu.title", "Menu.Title",
		      &(resource.mres.title.texture));
  readDatabaseTexture("menu.frame", "Menu.Frame",
		      &(resource.mres.frame.texture));
  readDatabaseColor("menu.title.textColor", "Menu.Title.TextColor",
		    &(resource.mres.title.textColor));
  readDatabaseColor("menu.frame.highlightColor",
			   "Menu.Frame.HighLightColor",
			   &(resource.mres.frame.hiColor));
  readDatabaseColor("menu.frame.textColor", "Menu.Frame.TextColor",
		    &(resource.mres.frame.textColor));
  readDatabaseColor("menu.frame.hiTextColor", "Menu.Frame.HiTextColor",
		    &(resource.mres.frame.hiTextColor));
  
  // toolbar configuration
  readDatabaseTexture("toolbar", "Toolbar",
		      &(resource.tres.toolbar.texture));
  readDatabaseTexture("toolbar.label", "Toolbar.Label",
		      &(resource.tres.label.texture));
  readDatabaseTexture("toolbar.clock", "Toolbar.Clock",
		      &(resource.tres.clock.texture));
  readDatabaseTexture("toolbar.button", "Toolbar.Button",
		      &(resource.tres.button.texture));
  readDatabaseTexture("toolbar.button.pressed", "Toolbar.Button.Pressed",
		      &(resource.tres.button.pressedTexture));
  readDatabaseColor("toolbar.textColor", "Toolbar.TextColor",
		    &(resource.tres.toolbar.textColor));

  // load border color
  readDatabaseColor("borderColor", "BorderColor", &(resource.border_color));
  
  // load bevel, border and handle widths
  if (XrmGetResource(resource.stylerc, "handleWidth", "HandleWidth",
                     &value_type, &value)) {
    if (sscanf(value.addr, "%u", &resource.handle_width) != 1)
      resource.handle_width = 8;
    else
      if (resource.handle_width > (getWidth() / 2) ||
          resource.handle_width == 0)
	resource.handle_width = 8;
  } else
    resource.handle_width = 8;

  if (XrmGetResource(resource.stylerc, "borderWidth", "BorderWidth",
                     &value_type, &value)) {
    if (sscanf(value.addr, "%u", &resource.border_width) != 1)
      resource.border_width = 1;
  } else
    resource.border_width = 1;

  if (XrmGetResource(resource.stylerc, "bevelWidth", "BevelWidth",
                     &value_type, &value)) {
    if (sscanf(value.addr, "%u", &resource.bevel_width) != 1)
      resource.bevel_width = 4;
    else
      if (resource.bevel_width > (getWidth() / 2) || resource.bevel_width == 0)
	resource.bevel_width = 4;
  } else
    resource.bevel_width = 4;

  if (XrmGetResource(resource.stylerc,
		     "titleJustify",
		     "TitleJustify", &value_type, &value)) {
    if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
      resource.justify = BScreen::RightJustify;
    else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
      resource.justify = BScreen::CenterJustify;
    else
      resource.justify = BScreen::LeftJustify;
  } else
    resource.justify = BScreen::LeftJustify;

  if (XrmGetResource(resource.stylerc,
		     "menuJustify",
		     "MenuJustify", &value_type, &value)) {
    if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
      resource.menu_justify = BScreen::RightJustify;
    else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
      resource.menu_justify = BScreen::CenterJustify;
    else
      resource.menu_justify = BScreen::LeftJustify;
  } else
    resource.menu_justify = BScreen::LeftJustify;

  if (XrmGetResource(resource.stylerc, "menu.bulletStyle", "Menu.BulletStyle",
                     &value_type, &value)) {
    if (! strncasecmp(value.addr, "empty", value.size))
      resource.menu_bullet_style = Basemenu::Empty;
    else if (! strncasecmp(value.addr, "square", value.size))
      resource.menu_bullet_style = Basemenu::Square;
    else if (! strncasecmp(value.addr, "triangle", value.size))
      resource.menu_bullet_style = Basemenu::Triangle;
    else if (! strncasecmp(value.addr, "diamond", value.size))
      resource.menu_bullet_style = Basemenu::Diamond;
    else
      resource.menu_bullet_style = Basemenu::Round;
  } else
    resource.menu_bullet_style = Basemenu::Round;

  if (XrmGetResource(resource.stylerc, "menu.bulletPosition",
                     "Menu.BulletPosition", &value_type, &value)) {
    if (! strncasecmp(value.addr, "right", value.size))
      resource.menu_bullet_pos = Basemenu::Right;
    else
      resource.menu_bullet_pos = Basemenu::Left;
  } else
    resource.menu_bullet_pos = Basemenu::Left;

  const char *defaultFont = "fixed";
  if (resource.font.title) {
    XFreeFont(getDisplay()->getDisplay(), resource.font.title);
    resource.font.title = 0;
  }

  if (XrmGetResource(resource.stylerc,
		     "titleFont",
		     "TitleFont", &value_type, &value)) {
    if ((resource.font.title = XLoadQueryFont(getDisplay()->getDisplay(),
                                              value.addr)) == NULL) {
      fprintf(stderr,
	      "BScreen::LoadStyle(): couldn't load font '%s'\n", value.addr);
      if ((resource.font.title = XLoadQueryFont(getDisplay()->getDisplay(),
                                                defaultFont)) == NULL) {
	fprintf(stderr, "BScreen::LoadStyle(): couldn't load default font.\n");
	exit(2);
      }  
    }
  } else {
    if ((resource.font.title = XLoadQueryFont(getDisplay()->getDisplay(),
                                              defaultFont)) == NULL) {
      fprintf(stderr, "BScreen::LoadStyle(): couldn't load default font\n");
      exit(2);
    }
  }

  if (resource.font.menu) {
    XFreeFont(getDisplay()->getDisplay(), resource.font.menu);
    resource.font.menu = 0;
  }

  if (XrmGetResource(resource.stylerc,
		     "menuFont",
		     "MenuFont", &value_type, &value)) {
    if ((resource.font.menu = XLoadQueryFont(getDisplay()->getDisplay(),
                                             value.addr)) == NULL) {
      fprintf(stderr,
	      "BScreen::LoadStyle(): couldn't load font '%s'\n", value.addr);
      if ((resource.font.menu = XLoadQueryFont(getDisplay()->getDisplay(),
                                               defaultFont)) == NULL) {
	fprintf(stderr, "BScreen::LoadStyle(): couldn't load default font.\n");
	exit(2);
      }  
    }
  } else {
    if ((resource.font.menu = XLoadQueryFont(getDisplay()->getDisplay(),
                                             defaultFont)) == NULL) {
      fprintf(stderr, "BScreen::LoadStyle: couldn't load default font.\n");
      exit(2);
    }
  }

  if (XrmGetResource(resource.stylerc,
                     "rootCommand",
                     "RootCommand", &value_type, &value)) {
#ifndef   __EMX__
    int dslen = strlen(DisplayString(getDisplay()->getDisplay()));
    
    char *displaystring = new char[dslen + 32];
    char *command = new char[strlen(value.addr) + dslen + 64];
    
    sprintf(displaystring, "%s", DisplayString(getDisplay()->getDisplay()));
    // gotta love pointer math
    sprintf(displaystring + dslen - 1, "%d", getScreenNumber());
    sprintf(command, "DISPLAY=%s exec %s &",  displaystring, value.addr);
    system(command);
    
    delete [] displaystring;
    delete [] command;
#else // !__EMX__
    spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", item->exec(), NULL);
#endif // __EMX__
  }
  
  XrmDestroyDatabase(resource.stylerc);
}


int BScreen::addWorkspace(void) {
  Workspace *wkspc = new Workspace(this, workspacesList->count());
  workspacesList->insert(wkspc);

  workspacemenu->insert(wkspc->getName(), wkspc->getMenu(),
			wkspc->getWorkspaceID() + 1);
  workspacemenu->update();

#ifdef    KDE
  unsigned long data = (unsigned long) workspacesList->count();
  
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(),
                  blackbox->getKWMNumberOfDesktopsAtom(),
                  blackbox->getKWMNumberOfDesktopsAtom(), 32,
                  PropModeReplace, (unsigned char *) &data, 1);

  sendToKWMModules(blackbox->getKWMModuleDesktopNumberChangeAtom(),
		   (XID) data);
#endif // KDE
 
  return workspacesList->count();
}


int BScreen::removeLastWorkspace(void) {
  if (workspacesList->count() > 1) {
    Workspace *wkspc = workspacesList->last();
    
    if (current_workspace->getWorkspaceID() == wkspc->getWorkspaceID())
      changeWorkspaceID(current_workspace->getWorkspaceID() - 1);
    
    wkspc->removeAll();
    
    workspacemenu->remove(wkspc->getWorkspaceID() + 2);
    workspacemenu->update();

    workspacesList->remove(wkspc);
    delete wkspc;
    
#ifdef    KDE
    unsigned long data = (unsigned long) workspacesList->count();
    
    XChangeProperty(getDisplay()->getDisplay(), getRootWindow(),
		    blackbox->getKWMNumberOfDesktopsAtom(),
		    blackbox->getKWMNumberOfDesktopsAtom(), 32,
		    PropModeReplace, (unsigned char *) &data, 1);
    
    sendToKWMModules(blackbox->getKWMModuleDesktopNumberChangeAtom(),
		     (XID) data);
#endif // KDE
    
    return workspacesList->count();
  }
  
  return 0;
}


void BScreen::addIcon(BlackboxIcon *icon) {
  iconmenu->insert(icon);
}


void BScreen::removeIcon(BlackboxIcon *icon) {
  iconmenu->remove(icon);
}


void BScreen::iconUpdate(void) {
  iconmenu->update();
}


Workspace *BScreen::getWorkspace(int w) {
  return workspacesList->find(w);
}


int BScreen::getCurrentWorkspaceID(void) {
  return current_workspace->getWorkspaceID();
}


void BScreen::changeWorkspaceID(int id) {
  if (current_workspace && id != current_workspace->getWorkspaceID()) {
    current_workspace->hideAll();
    if (blackbox->getFocusedWindow() &&
        (! blackbox->getFocusedWindow()->isStuck()))
      current_workspace->setFocusWindow(-1);
    current_workspace = getWorkspace(id);
    
    toolbar->redrawWorkspaceLabel(True);
    workspacemenu->setHighlight(id + 2);
    
    current_workspace->showAll();
  }

#ifdef    KDE
  unsigned long data = (unsigned long) current_workspace->getWorkspaceID() + 1;
  
  XChangeProperty(getDisplay()->getDisplay(), getRootWindow(),
                  blackbox->getKWMCurrentDesktopAtom(),
                  blackbox->getKWMCurrentDesktopAtom(), 32, PropModeReplace,
                  (unsigned char *) &data, 1);
  
  sendToKWMModules(blackbox->getKWMModuleDesktopChangeAtom(), (XID) data);
#endif // KDE
}


void BScreen::raiseWindows(Window *workspace_stack, int num) {
  Window *session_stack = new
    Window[(num + workspacesList->count() + rootmenuList->count() + 6)];
  int i = 0, k = num;
  
  blackbox->grab();
  
  XRaiseWindow(getDisplay()->getDisplay(), iconmenu->getWindowID());
  *(session_stack + i++) = iconmenu->getWindowID();
  
  LinkedListIterator<Workspace> wit(workspacesList);
  for (; wit.current(); wit++)
    *(session_stack + i++) = wit.current()->getMenu()->getWindowID();
 
  *(session_stack + i++) = workspacemenu->getWindowID();
  *(session_stack + i++) = slit->getMenu()->getWindowID();

  LinkedListIterator<Rootmenu> rit(rootmenuList);
  for (; rit.current(); rit++)
    *(session_stack + i++) = rit.current()->getWindowID();
  *(session_stack + i++) = rootmenu->getWindowID();

  if (toolbar->isOnTop()) {
    *(session_stack + i++) = toolbar->getWindowID();
    *(session_stack + i++) = slit->getWindowID();
  }
  
  while (k--)
    *(session_stack + i++) = *(workspace_stack + k);
  
  XRestackWindows(getDisplay()->getDisplay(), session_stack, i);
  
  blackbox->ungrab();
  
  delete [] session_stack;
}


#ifdef    HAVE_STRFTIME
void BScreen::saveStrftimeFormat(char *format) {
  if (resource.strftime_format) delete [] resource.strftime_format;
  
  resource.strftime_format = new char[strlen(format) + 1];
  sprintf(resource.strftime_format, "%s", format);
}
#endif // HAVE_STRFTIME


void BScreen::addWorkspaceName(char *name) {
  int nlen = strlen(name) + 1;
  char *wkspc_name = new char[nlen];
  strncpy(wkspc_name, name, nlen);
  
  workspaceNames->insert(wkspc_name);
}


void BScreen::getNameOfWorkspace(int id, char **name) {
  if (id >= 0 && id < workspaceNames->count()) {
    char *wkspc_name = workspaceNames->find(id);
    
    if (wkspc_name) {
      int len = strlen(wkspc_name) + 1;
      *name = new char [len];
      sprintf(*name, "%s", wkspc_name);
    }
  } else
    *name = 0;
}


void BScreen::reassociateWindow(BlackboxWindow *w) {
  if (! w->isStuck() && w->getWorkspaceNumber() != getCurrentWorkspaceID()) {
    getWorkspace(w->getWorkspaceNumber())->removeWindow(w);
    getCurrentWorkspace()->addWindow(w);
  }
}


void BScreen::nextFocus(void) {
  Bool have_focused = False;
  int focused_window_number = -1;

  if (blackbox->getFocusedWindow())
    if (blackbox->getFocusedWindow()->getScreen()->getScreenNumber() ==
	getScreenNumber()) {
      have_focused = True;
      focused_window_number = blackbox->getFocusedWindow()->getWindowNumber();
    }
  
  if ((getCurrentWorkspace()->getCount() > 1) && have_focused) {
    BlackboxWindow *next;
    
    int next_window_number = focused_window_number;
    do {
      do {
	if ((++next_window_number) >= getCurrentWorkspace()->getCount())
	  next_window_number = 0;
	
	next = getCurrentWorkspace()->getWindow(next_window_number);
      }	while (next->isIconic() && (next_window_number !=
				    focused_window_number));
    } while ((! next->setInputFocus()) && (next_window_number !=
					   focused_window_number));
    
    if (next_window_number != focused_window_number)
      getCurrentWorkspace()->raiseWindow(next);
  } else if (getCurrentWorkspace()->getCount() >= 1) {
    getCurrentWorkspace()->getWindow(0)->setInputFocus();
  }
}


void BScreen::prevFocus(void) {
  Bool have_focused = False;
  int focused_window_number = -1;
  
  if (blackbox->getFocusedWindow())
    if (blackbox->getFocusedWindow()->getScreen()->getScreenNumber() ==
	getScreenNumber()) {
      have_focused = True;
      focused_window_number = blackbox->getFocusedWindow()->getWindowNumber();
    }
  
  if ((getCurrentWorkspace()->getCount() > 1) && have_focused) {
    BlackboxWindow *prev;
    
    int prev_window_number = focused_window_number;
    do {
      do {
	if ((--prev_window_number) < 0)
	  prev_window_number = getCurrentWorkspace()->getCount() - 1;
	
	prev = getCurrentWorkspace()->getWindow(prev_window_number);
      } while (prev->isIconic() && (prev_window_number !=
				    focused_window_number));
    } while ((! prev->setInputFocus()) && (prev_window_number !=
					   focused_window_number));
    
    if (prev_window_number != focused_window_number)
      getCurrentWorkspace()->raiseWindow(prev);
  } else if (getCurrentWorkspace()->getCount() >= 1)
    getCurrentWorkspace()->getWindow(0)->setInputFocus();
}


void BScreen::raiseFocus(void) {
  Bool have_focused = False;
  int focused_window_number = -1;
  
  if (blackbox->getFocusedWindow())
    if (blackbox->getFocusedWindow()->getScreen()->getScreenNumber() ==
	getScreenNumber()) {
      have_focused = True;
      focused_window_number = blackbox->getFocusedWindow()->getWindowNumber();
    }
  
  if ((getCurrentWorkspace()->getCount() > 1) && have_focused)
    getWorkspace(blackbox->getFocusedWindow()->getWorkspaceNumber())->
      raiseWindow(blackbox->getFocusedWindow()); 
}


void BScreen::InitMenu(void) {
  if (rootmenu) {
    while (rootmenuList->count())
      rootmenuList->remove(0);
    
    while (rootmenu->getCount())
      rootmenu->remove(0);
  } else
    rootmenu = new Rootmenu(blackbox, this);
  
  Bool defaultMenu = True;
  
  if (blackbox->getMenuFilename()) {
    FILE *menu_file = fopen(blackbox->getMenuFilename(), "r");

    if (menu_file) {
      if (! feof(menu_file)) {
	char line[1024], label[1024];
	memset(line, 0, 1024);
	memset(label, 0, 1024);
	
	while (fgets(line, 1024, menu_file) && ! feof(menu_file)) {
	  if (line[0] != '#') {
	    int i, key = 0, index = -1, len = strlen(line);
	    
	    key = 0;
	    for (i = 0; i < len; i++)
	      if (line[i] == '[') index = 0;
	      else if (line[i] == ']') break;
	      else if (line[i] != ' ')
		if (index++ >= 0)
		  key += line[i] | 0x20;
	    
	    if (key == 517) {
	      index = -1;
	      for (i = index; i < len; i++)
		if (line[i] == '(') index = 0;
		else if (line[i] == ')') break;
		else if (index++ >= 0) {
		  if (line[i] == '\\' && i < len - 1) i++;
		  label[index - 1] = line[i];
		}
	      
	      if (index == -1) index = 0;
	      label[index] = '\0';
	      
	      rootmenu->setLabel(label);
	      defaultMenu = parseMenuFile(menu_file, rootmenu);
	      break;
	    }
	  }
	}
      } else
	fprintf(stderr, "%s: Empty menu file", blackbox->getMenuFilename());
      
      fclose(menu_file);
    } else
      perror(blackbox->getMenuFilename());
  }
  
  if (defaultMenu) {
    rootmenu->defaultMenu();
    rootmenu->insert("xterm", BScreen::Execute, "xterm");
    rootmenu->insert("Restart", BScreen::Restart);
    rootmenu->insert("Exit", BScreen::Exit);
  } else
    blackbox->saveMenuFilename(blackbox->getMenuFilename());
}


Bool BScreen::parseMenuFile(FILE *file, Rootmenu *menu) {
  char line[1024], label[1024], command[1024];
  
  while (! feof(file)) {
    memset(line, 0, 1024);
    memset(label, 0, 1024);
    memset(command, 0, 1024);
    
    if (fgets(line, 1024, file)) {
      if (line[0] != '#') {
	register int i, key = 0, parse = 0, index = -1,
          line_length = strlen(line), label_length = 0, command_length = 0;
	
	// determine the keyword
	key = 0;
	for (i = 0; i < line_length; i++)
	  if (line[i] == '[') parse = 1;
	  else if (line[i] == ']') break;
	  else if (line[i] != ' ')
	    if (parse)
	      key += line[i] | 0x20;
	
	// get the label enclosed in ()'s
	parse = 0;
	for (i = 0; i < line_length; i++)
	  if (line[i] == '(') {
	    index = 0;
	    parse = 1;
	  } else if (line[i] == ')') break;
	  else if (index++ >= 0) {
	    if (line[i] == '\\' && i < line_length - 1) i++;
	    label[index - 1] = line[i];
	  }
	
	if (parse) {
	  label[index] = '\0';
	  label_length = index;
	} else {
	  label[0] = '\0';
	  label_length = 0;
	}
	
	// get the command enclosed in {}'s
	parse = 0;
	index = -1;
	for (i = 0; i < line_length; i++)
	  if (line[i] == '{') {
	    index = 0;
	    parse = 1;
	  } else if (line[i] == '}') break;
	  else if (index++ >= 0) {
	    if (line[i] == '\\' && i < line_length - 1) i++;
	    command[index - 1] = line[i];
	  }
	
	if (parse) {
	  command[index] = '\0';
	  command_length = index;
	} else {
	  command[0] = '\0';
	  command_length = 0;
	}
	
	switch (key) {
        case 311: //end
          return ((menu->getCount() == 0) ? True : False);
	  
          break;
	  
        case 333: // nop
	  menu->insert(label);

	  break;

	case 421: // exec 
	  if ((! *label) && (! *command)) {
	    fprintf(stderr," BScreen::parseMenuFile: [exec] error, "
		    "no menu label and/or command defined\n");
	    continue;
	  }
	  
	  menu->insert(label, BScreen::Execute, command);
	  
	  break;
	  
	case 442: // exit
	  if (! *label) {
	      fprintf(stderr, "BScreen::parseMenuFile: [exit] error, "
		      "no menu label defined\n");
	      continue;
	  }
	  
	  menu->insert(label, BScreen::Exit);
	  
	  break;

	case 561: // style
	  {
	    if ((! *label) || (! *command)) {
	      fprintf(stderr, "BScreen::parseMenuFile: [style] error, "
		      "no menu label and/or filename defined\n");
	      continue;
	    }
	    
	    char style[MAXPATHLEN];

	    // perform shell style ~ home directory expansion
	    char *homedir = 0;
	    int homedir_len = 0;
	    if (*command == '~' && *(command + 1) == '/') {
	      homedir = getenv("HOME");
	      homedir_len = strlen(homedir);
	    }
	    	    
	    if (homedir && homedir_len != 0) {
	      strncpy(style, homedir, homedir_len);
	      
	      strncpy(style + homedir_len, command + 1,
		      command_length - 1);
	      *(style + command_length + homedir_len - 1) = '\0';
	    } else {
	      strncpy(style, command, command_length);
	      *(style + command_length) = '\0';
	    }
	    
	    menu->insert(label, BScreen::SetStyle, style);
	  }
	  
	  break;
	  
	case 740: // include
	  {
	    if (! *label) {
	      fprintf(stderr, "BScreen::parseMenuFile: [include] error, "
		      "no filename defined\n");
	      continue;
	    }
	    
	    char newfile[MAXPATHLEN];

	    // perform shell style ~ home directory expansion
	    char *homedir = 0;
	    int homedir_len = 0;
	    if (*label == '~' && *(label + 1) == '/') {
	      homedir = getenv("HOME");
	      homedir_len = strlen(homedir);
	    }
	    
	    if (homedir && homedir_len != 0) {
	      strncpy(newfile, homedir, homedir_len);
	      
	      strncpy(newfile + homedir_len, label + 1,
		      label_length - 1);
	      *(newfile + label_length + homedir_len - 1) = '\0';
	    } else {
	      strncpy(newfile, label, label_length);
	      *(newfile + label_length) = '\0';
	    }
	    
	    if (newfile) {
	      FILE *submenufile = fopen(newfile, "r");
	      
	      if (submenufile) {
		if (! feof(submenufile)) {
		  if (! parseMenuFile(submenufile, menu))
		    blackbox->saveMenuFilename(newfile);
		  
		  fclose(submenufile);
		}
	      } else
		perror(newfile);
	    }
	  }
	  
	  break;
	  
	  case 767: // submenu
	    {
	      if (! *label) {
		fprintf(stderr, "BScreen::parseMenuFile: [submenu] error, "
			"no menu label defined\n");
		continue;
	      }

              Rootmenu *submenu = new Rootmenu(blackbox, this);
	      
	      if (*command)
		submenu->setLabel(command);
	      else
		submenu->setLabel(label);
	      
	      parseMenuFile(file, submenu);
	      submenu->update();
	      menu->insert(label, submenu);
	      rootmenuList->insert(submenu);
	    }
	    
	    break;

	case 773: // restart
	  {
	    if (! *label) {
	      fprintf(stderr, "BScreen::parseMenuFile: [restart] error, "
		      "no menu label defined\n");
	      continue;
	    }
	    
	    if (*command)
	      menu->insert(label, BScreen::RestartOther, command);
	    else
	      menu->insert(label, BScreen::Restart);
	  }
	  
	  break;
	  
	  case 845: // reconfig
	    {
	      if (! *label) {
		fprintf(stderr, "BScreen::parseMenuFile: [reconfig] error, "
			"no menu label defined\n");
		continue;
	      }
	      
	      menu->insert(label, BScreen::Reconfigure);
	    }
	    
	    break;
	    
	case 1090: // workspaces
	  {
	    if (! *label) {
	      fprintf(stderr, "BScreen:parseMenuFile: [workspaces] error, "
		      "no menu label defined\n");
	      continue;
	    }
	    
	    menu->insert(label, workspacemenu);
	      
	    break;
	  }
	}
      }
    }
  }
  
  return ((menu->getCount() == 0) ? True : False);
}


void BScreen::shutdown(void) {
  blackbox->grab();

  XSelectInput(getDisplay()->getDisplay(), getRootWindow(), NoEventMask);
  XSync(getDisplay()->getDisplay(), False);

#ifdef    KDE
  XDeleteProperty(getDisplay()->getDisplay(), getRootWindow(),
                  blackbox->getKWMRunningAtom());
#endif // KDE
  
  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it ++)
    it.current()->shutdown();
  
  blackbox->ungrab();
}


void BScreen::showPosition(int x, int y) {
  if (! geom_visible) {
    XMoveResizeWindow(getDisplay()->getDisplay(), geom_window,
                      (getWidth() - geom_w) / 2,
                      (getHeight() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(getDisplay()->getDisplay(), geom_window);
    XRaiseWindow(getDisplay()->getDisplay(), geom_window);

    geom_visible = True;
  }

  char label[1024];

  sprintf(label, "X: %4d x Y: %4d", x, y);

  XClearWindow(getDisplay()->getDisplay(), geom_window);
  XDrawString(getDisplay()->getDisplay(), geom_window, wfocusGC,
              resource.bevel_width, getTitleFont()->ascent +
              resource.bevel_width, label, strlen(label));
}


void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
  if (! geom_visible) {
    XMoveResizeWindow(getDisplay()->getDisplay(), geom_window,
                      (getWidth() - geom_w) / 2,
                      (getHeight() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(getDisplay()->getDisplay(), geom_window);
    XRaiseWindow(getDisplay()->getDisplay(), geom_window);

    geom_visible = True;
  }

  char label[1024];

  sprintf(label, "W: %4d x H: %4d", gx, gy);

  XClearWindow(getDisplay()->getDisplay(), geom_window);
  XDrawString(getDisplay()->getDisplay(), geom_window, wfocusGC,
              resource.bevel_width, getTitleFont()->ascent +
              resource.bevel_width, label, strlen(label));
}


void BScreen::hideGeometry(void) {
  if (geom_visible) {
    XUnmapWindow(getDisplay()->getDisplay(), geom_window);
    geom_visible = False;
  }
}


#ifdef    KDE
Bool BScreen::isKWMModule(Window win) {
  Atom type_return;

  int ijunk;
  unsigned long uljunk, return_value = 0, *result;

  if (XGetWindowProperty(getDisplay()->getDisplay(), win,
                         blackbox->getKWMModuleAtom(), 0l, 1l,
                         False, blackbox->getKWMModuleAtom(), &type_return,
                         &ijunk, &uljunk, &uljunk,
                         (unsigned char **) &result) == Success && result) {
    return_value = *result;
    XFree((char *) result);
  }

  return (return_value != 0);
}


void BScreen::sendToKWMModules(Atom atom, XID data) {
  LinkedListIterator<Window> it(kwm_module_list);
  for (; it.current(); it++)
    sendClientMessage(*(it.current()), atom, data);
}


void BScreen::addKWMWindow(Window win) {
  Window *new_win = new Window;
  *new_win = win;

  kwm_window_list->insert(new_win);
  sendToKWMModules(blackbox->getKWMModuleWinAddAtom(), win);
}


void BScreen::removeKWMWindow(Window win) {
  LinkedListIterator<Window> mod_it(kwm_module_list);
  for (; mod_it.current(); mod_it++)
    if (*(mod_it.current()) == win)
      break;
  
  if (mod_it.current()) {
    Window *w = mod_it.current();

    kwm_module_list->remove(w);
    delete w;
  }
  
  LinkedListIterator<Window> win_it(kwm_window_list);
  for (; win_it.current(); win_it++)
    if (*(win_it.current()) == win)
      break;
  
  if (win_it.current()) {
    Window *w = win_it.current();

    kwm_window_list->remove(w);
    delete w;

    sendToKWMModules(blackbox->getKWMModuleWinRemoveAtom(), win);
  }
}


void BScreen::sendToKWMModules(XClientMessageEvent *e) {
  LinkedListIterator<Window> it(kwm_module_list);
  for (; it.current(); it++) {
    e->window = *(it.current());
    XSendEvent(getDisplay()->getDisplay(), *(it.current()), False,
               0 | ((*(it.current()) == getRootWindow()) ?
                    SubstructureRedirectMask : 0), (XEvent *) e);
  }
}


void BScreen::sendClientMessage(Window window, Atom atom, XID data) {
  XEvent e;
  unsigned long mask;
  
  e.xclient.type = ClientMessage;
  e.xclient.window = window;
  e.xclient.message_type = atom;
  e.xclient.format = 32;
  e.xclient.data.l[0] = (unsigned long) data;
  e.xclient.data.l[1] = CurrentTime;
  
  mask = 0 | ((window == getRootWindow()) ? SubstructureRedirectMask : 0);
  XSendEvent(getDisplay()->getDisplay(), window, False, mask, &e);
}


void BScreen::addKWMModule(Window win) {
  if (isKWMModule(win) && blackbox->validateWindow(win)) {
    Window *new_win = new Window;
    *new_win = win;
    
    kwm_module_list->insert(new_win);
    sendClientMessage(win, blackbox->getKWMModuleInitAtom(), 0);

    LinkedListIterator<Window> it(kwm_window_list);
    for (; it.current(); it++)
      sendClientMessage(win, blackbox->getKWMModuleWinAddAtom(),
			(XID) *(it.current()));
    
    sendClientMessage(win, blackbox->getKWMModuleInitializedAtom(), 0);

    XSelectInput(getDisplay()->getDisplay(), win, StructureNotifyMask);
  } else
    removeKWMWindow(win);
}


void BScreen::scanWorkspaceNames(void) {
  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it++)
    it.current()->rereadName();
}
#endif // KDE


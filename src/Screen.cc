//
// Screen.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@tcac.net
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <X11/Xatom.h>

#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Icon.hh"
#include "Image.hh"
#include "Screen.hh"
#include "Rootmenu.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#ifdef STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif


static Bool running = True;

static int anotherWMRunning(Display *display, XErrorEvent *) {
  fprintf(stderr,
	  "BScreen::BScreen: an error occured while querying the X server.\n"
	  "  another window manager already running on display %s.\n",
          DisplayString(display));

  running = False;

  return(-1);
}


BScreen::BScreen(Blackbox *bb, int scrn) {
  blackbox = bb;
  screen_number = scrn;
  display = blackbox->getDisplay();

  event_mask = ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
    SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
    ButtonPressMask | ButtonReleaseMask;
  
  root_window = RootWindow(display, screen_number);
  visual = DefaultVisual(display, screen_number);
  depth = DefaultDepth(display, screen_number);
  
  XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(display, root_window, event_mask);
  XSync(display, False);
  XSetErrorHandler((XErrorHandler) old);

  managed = running;
  if (! managed) return;
  
  rootmenu = 0;
  resource.stylerc = 0;
  resource.font.menu = resource.font.title = (XFontStruct *) 0;
 
#ifdef HAVE_STRFTIME
  resource.strftime_format = 0;
#endif
  
  resource.wres.decoration.focusTexture.color.allocated =
    resource.wres.decoration.focusTexture.colorTo.allocated =
    resource.wres.decoration.focusTexture.hiColor.allocated =
    resource.wres.decoration.focusTexture.loColor.allocated =

    resource.wres.decoration.unfocusTexture.color.allocated =
    resource.wres.decoration.unfocusTexture.colorTo.allocated =
    resource.wres.decoration.unfocusTexture.hiColor.allocated =
    resource.wres.decoration.unfocusTexture.loColor.allocated =

    resource.wres.decoration.focusTextColor.allocated =
    resource.wres.decoration.unfocusTextColor.allocated =

    resource.wres.button.focusTexture.color.allocated =
    resource.wres.button.focusTexture.colorTo.allocated =
    resource.wres.button.focusTexture.hiColor.allocated =
    resource.wres.button.focusTexture.loColor.allocated =

    resource.wres.button.unfocusTexture.color.allocated =
    resource.wres.button.unfocusTexture.colorTo.allocated =
    resource.wres.button.unfocusTexture.hiColor.allocated =
    resource.wres.button.unfocusTexture.loColor.allocated =

    resource.wres.button.pressedTexture.color.allocated =
    resource.wres.button.pressedTexture.colorTo.allocated =
    resource.wres.button.pressedTexture.hiColor.allocated =
    resource.wres.button.pressedTexture.loColor.allocated =

    resource.wres.frame.texture.color.allocated =
    resource.wres.frame.texture.colorTo.allocated =
    resource.wres.frame.texture.hiColor.allocated =
    resource.wres.frame.texture.loColor.allocated =

    resource.tres.toolbar.textColor.allocated =
    resource.tres.toolbar.texture.color.allocated =
    resource.tres.toolbar.texture.colorTo.allocated =
    resource.tres.toolbar.texture.hiColor.allocated =
    resource.tres.toolbar.texture.loColor.allocated =

    resource.tres.label.texture.color.allocated =
    resource.tres.label.texture.colorTo.allocated =
    resource.tres.label.texture.hiColor.allocated =
    resource.tres.label.texture.loColor.allocated =

    resource.tres.button.texture.color.allocated =
    resource.tres.button.texture.colorTo.allocated =
    resource.tres.button.texture.hiColor.allocated =
    resource.tres.button.texture.loColor.allocated =

    resource.tres.button.pressedTexture.color.allocated =
    resource.tres.button.pressedTexture.colorTo.allocated =
    resource.tres.button.pressedTexture.hiColor.allocated =
    resource.tres.button.pressedTexture.loColor.allocated =

    resource.tres.clock.texture.color.allocated =
    resource.tres.clock.texture.colorTo.allocated =
    resource.tres.clock.texture.hiColor.allocated =
    resource.tres.clock.texture.loColor.allocated =

    resource.mres.title.textColor.allocated =
    resource.mres.title.texture.color.allocated =
    resource.mres.title.texture.colorTo.allocated =
    resource.mres.title.texture.hiColor.allocated =
    resource.mres.title.texture.loColor.allocated =

    resource.mres.frame.hiColor.allocated =
    resource.mres.frame.textColor.allocated =
    resource.mres.frame.hiTextColor.allocated =
    resource.mres.frame.texture.color.allocated =
    resource.mres.frame.texture.colorTo.allocated =
    resource.mres.frame.texture.hiColor.allocated =
    resource.mres.frame.texture.loColor.allocated =

    resource.border_color.allocated = 0;
  
  XDefineCursor(display, root_window, blackbox->getSessionCursor());

  xres = WidthOfScreen(ScreenOfDisplay(display, screen_number));
  yres = HeightOfScreen(ScreenOfDisplay(display, screen_number));

  workspaceNames = new LinkedList<char>;
  workspacesList = new LinkedList<Workspace>;

#ifdef    KDE
  kwm_module_list = new LinkedList<Window>;
  kwm_window_list = new LinkedList<Window>;

  {
    unsigned long data = 1;
    XChangeProperty(display, root_window, blackbox->getKWMRunningAtom(),
                    blackbox->getKWMRunningAtom(), 32, PropModeAppend,
                    (unsigned char *) &data, 1);

    Atom a;
    a = XInternAtom(display, "KWM_STRING_MAXIMIZE", False);
    XChangeProperty(display, root_window, a, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) "&Maximize", 10);

    a = XInternAtom(display, "KWM_STRING_UNMAXIMIZE", False);
    XChangeProperty(display, root_window, a, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) "&Un Maximize", 12);

    a = XInternAtom(display, "KWM_STRING_ICONIFY", False);
    XChangeProperty(display, root_window, a, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) "&Iconify", 9);

    a = XInternAtom(display, "KWM_STRING_UNICONIFY", False);
    XChangeProperty(display, root_window, a, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) "&De Iconify", 9);

    a = XInternAtom(display,"KWM_STRING_STICKY",False);
    XChangeProperty(display, root_window, a, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) "&Stick", 7);

    a = XInternAtom(display, "KWM_STRING_UNSTICKY", False);
    XChangeProperty(display, root_window, a, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) "&Un Stick", 9);
    
    a = XInternAtom(display, "KWM_STRING_MOVE", False);
    XChangeProperty(display, root_window, a, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) "&Move", 6);

    a = XInternAtom(display, "KWM_STRING_RESIZE", False);
    XChangeProperty(display, root_window, a, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) "&Resize", 8);

    a = XInternAtom(display, "KWM_STRING_CLOSE", False);
    XChangeProperty(display, root_window, a, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) "&Close", 7);

    a = XInternAtom(display, "KWM_STRING_ONTOCURRENTDESKTOP", False);
    XChangeProperty(display, root_window, a, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) "R&eassociate with current...", 7);
  }
#endif // KDE
  
  image_control = new BImageControl(blackbox, this);
  image_control->installRootColormap();
  root_colormap_installed = True;

  blackbox->load_rc(this);

  LoadStyle();
  
  XGCValues gcv;
  gcv.foreground = image_control->getColor("white");
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.subwindow_mode = IncludeInferiors;
  opGC = XCreateGC(display, root_window,
		   GCForeground|GCFunction|GCSubwindowMode, &gcv);

  gcv.foreground = getWResource()->decoration.unfocusTextColor.pixel;
  gcv.font = getTitleFont()->fid;
  wunfocusGC = XCreateGC(display, root_window, GCForeground|GCBackground|
		       GCFont, &gcv);
  
  gcv.foreground = getWResource()->decoration.focusTextColor.pixel;
  gcv.font = getTitleFont()->fid;
  wfocusGC = XCreateGC(display, root_window, GCForeground|GCBackground|
		       GCFont, &gcv);

  gcv.foreground = getMResource()->title.textColor.pixel;
  gcv.font = getTitleFont()->fid;
  mtitleGC = XCreateGC(display, root_window, GCForeground|GCFont, &gcv);

  gcv.foreground = getMResource()->frame.textColor.pixel;
  gcv.font = getMenuFont()->fid;
  mframeGC = XCreateGC(display, root_window, GCForeground|GCFont, &gcv);

  gcv.foreground = getMResource()->frame.hiTextColor.pixel;
  mhiGC = XCreateGC(display, root_window, GCForeground|GCBackground|GCFont,
                      &gcv);

  gcv.foreground = getMResource()->frame.hiColor.pixel;
  gcv.arc_mode = ArcChord;
  gcv.fill_style = FillSolid;
  mhbgGC = XCreateGC(display, root_window, GCForeground|GCFillStyle|GCArcMode,
                    &gcv);
  
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
  {
    unsigned long data = (unsigned long) workspacesList->count();
    
    XChangeProperty(display, root_window,
		    blackbox->getKWMNumberOfDesktopsAtom(),
		    blackbox->getKWMNumberOfDesktopsAtom(), 32,
		    PropModeReplace, (unsigned char *) &data, 1);

    sendToKWMModules(blackbox->getKWMModuleDesktopNumberChangeAtom(),
		     (XID) data);
  }
#endif // KDE
  
  workspacemenu->insert("Icons", iconmenu);
  workspacemenu->update();
  
  current_workspace = workspacesList->first();
  workspacemenu->setHighlight(2);

  toolbar = new Toolbar(blackbox, this);

  InitMenu();

  raiseWindows(0, 0);
  rootmenu->update();

  changeWorkspaceID(0);

  unsigned int nchild;
  Window r, p, *children;
  XQueryTree(display, root_window, &r, &p, &children, &nchild);
  
  for (int i = 0; i < (int) nchild; ++i) {
    if (children[i] == None ||
	(! blackbox->validateWindow(children[i])))
      continue;
    
    XWindowAttributes attrib;
    if (XGetWindowAttributes(display, children[i], &attrib)) {
#ifdef    KDE
      addKWMModule(children[i]);
#endif // KDE
      if (! attrib.override_redirect && attrib.map_state != IsUnmapped) {
	BlackboxWindow *nWin = new BlackboxWindow(blackbox, this, children[i]);
	
	XMapRequestEvent mre;
	mre.window = children[i];
	nWin->mapRequestEvent(&mre);
      }
    }
  }
  
  XSync(display, False);
}


BScreen::~BScreen(void) {
  if (! managed) return;

#ifdef    HAVE_STRFTIME
  if (resource.strftime_format) delete [] resource.strftime_format;
#endif // HAVE_STRFTIME

  delete rootmenu;
  delete toolbar;
  delete image_control;

  delete workspacesList;
  delete workspaceNames;

#ifdef    KDE
  delete kwm_module_list;
  delete kwm_window_list;
#endif // KDE

  if (resource.font.title) {
    XFreeFont(display, resource.font.title);
    resource.font.title = 0;
  }
  
  if (resource.font.menu) {
    XFreeFont(display, resource.font.menu);
    resource.font.menu = 0;
  }
  
  XFreeGC(display, opGC);
  XFreeGC(display, wfocusGC);
  XFreeGC(display, wunfocusGC);
  XFreeGC(display, mtitleGC);
  XFreeGC(display, mframeGC);
  XFreeGC(display, mhiGC);
  XFreeGC(display, mhbgGC);
}

void BScreen::readDatabaseTexture(char *rname, char *rclass,
				  BTexture *texture)
{
  XrmValue value;
  char *value_type;
  
  texture->texture = 0;
  
  if (XrmGetResource(resource.stylerc, rname, rclass, &value_type,
		     &value)) {
    if (strstr(value.addr, "Solid")) {
      texture->texture |= BImage_Solid;
    } else if (strstr(value.addr, "Gradient")) {
      texture->texture |= BImage_Gradient;
      
      if (strstr(value.addr, "Diagonal")) {
	texture->texture |= BImage_Diagonal;
      } else if (strstr(value.addr, "Horizontal")) {
	texture->texture |= BImage_Horizontal;
      } else if (strstr(value.addr, "Vertical")) {
	texture->texture |= BImage_Vertical;
      } else
	texture->texture |= BImage_Diagonal;
    } else
      texture->texture |= BImage_Solid;
    
    if (strstr(value.addr, "Raised"))
      texture->texture |= BImage_Raised;
    else if (strstr(value.addr, "Sunken"))
      texture->texture |= BImage_Sunken;
    else if (strstr(value.addr, "Flat"))
      texture->texture |= BImage_Flat;
    else
      texture->texture |= BImage_Raised;
    
    if (! (texture->texture & BImage_Flat))
      if (strstr(value.addr, "Bevel"))
	if (strstr(value.addr, "Bevel1"))
	  texture->texture |= BImage_Bevel1;
	else if (strstr(value.addr, "Bevel2"))
	  texture->texture |= BImage_Bevel2;
	else
	  texture->texture |= BImage_Bevel1;
  }
  
  if (texture->texture & BImage_Solid) {
    int clen = strlen(rclass) + 8, nlen = strlen(rname) + 8;
    char *colorclass = new char[clen], *colorname = new char[nlen];
    
    sprintf(colorclass, "%s.Color", rclass);
    sprintf(colorname,  "%s.color", rname);
    
    readDatabaseColor(colorname, colorclass, &(texture->color));
    
    delete [] colorclass;
    delete [] colorname;
    
    if ((! texture->color.allocated) ||
	(texture->texture & BImage_Flat))
      return;
    
    XColor xcol;
    
    xcol.red = (unsigned int) (texture->color.red +
			       (texture->color.red >> 1));
    if (xcol.red >= 0xff) xcol.red = 0xffff;
    else xcol.red *= 0xff;
    xcol.green = (unsigned int) (texture->color.green +
				 (texture->color.green >> 1));
    if (xcol.green >= 0xff) xcol.green = 0xffff;
    else xcol.green *= 0xff;
    xcol.blue = (unsigned int) (texture->color.blue +
				(texture->color.blue >> 1));
    if (xcol.blue >= 0xff) xcol.blue = 0xffff;
    else xcol.blue *= 0xff;
    
    if (! XAllocColor(display, image_control->getColormap(), &xcol))
      xcol.pixel = 0;
    
    texture->hiColor.pixel = xcol.pixel;
    
    xcol.red = (unsigned int) ((texture->color.red >> 2) +
			       (texture->color.red >> 1)) * 0xff;
    xcol.green = (unsigned int) ((texture->color.green >> 2) +
				 (texture->color.green >> 1)) * 0xff;
    xcol.blue = (unsigned int) ((texture->color.blue >> 2) +
				(texture->color.blue >> 1)) * 0xff;
    
    if (! XAllocColor(display, image_control->getColormap(), &xcol))
      xcol.pixel = 0;
    
    texture->loColor.pixel = xcol.pixel;
  } else if (texture->texture & BImage_Gradient) {
    int clen = strlen(rclass) + 10, nlen = strlen(rname) + 10;
    char *colorclass = new char[clen], *colorname = new char[nlen],
      *colortoclass = new char[clen], *colortoname = new char[nlen];
    
    sprintf(colorclass, "%s.Color", rclass);
    sprintf(colorname,  "%s.color", rname);
    
    sprintf(colortoclass, "%s.ColorTo", rclass);
    sprintf(colortoname,  "%s.colorTo", rname);
    
    readDatabaseColor(colorname, colorclass, &(texture->color));
    readDatabaseColor(colortoname, colortoclass, &(texture->colorTo));
    
    delete [] colorclass;
    delete [] colorname;
    delete [] colortoclass;
    delete [] colortoname;
  }
}


void BScreen::readDatabaseColor(char *rname, char *rclass, BColor *color) {
  XrmValue value;
  char *value_type;
  
  if (color->allocated) {
    XFreeColors(display, image_control->getColormap(),
		&(color->pixel), 1, 0);
    color->allocated = False;
  }
  
  if (XrmGetResource(resource.stylerc, rname, rclass, &value_type,
		     &value)) {
    color->pixel = image_control->getColor(value.addr, &color->red,
					   &color->green, &color->blue);
    color->allocated = 1;
  } else {
    color->pixel = color->red = color->green = color->blue = 0;
  }
}


void BScreen::reconfigure(void) {
  LoadStyle();

  XGCValues gcv;
  gcv.foreground = image_control->getColor("white");
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.subwindow_mode = IncludeInferiors;
  XChangeGC(display, opGC, GCForeground|GCFunction|GCSubwindowMode, &gcv);

  gcv.foreground = getWResource()->decoration.unfocusTextColor.pixel;
  gcv.font = getTitleFont()->fid;
  XChangeGC(display, wunfocusGC, GCForeground|GCBackground| GCFont, &gcv);

  gcv.foreground = getWResource()->decoration.focusTextColor.pixel;
  gcv.font = getTitleFont()->fid;
  XChangeGC(display, wfocusGC, GCForeground|GCBackground| GCFont, &gcv);

  gcv.foreground = getMResource()->title.textColor.pixel;
  gcv.font = getTitleFont()->fid;
  XChangeGC(display, mtitleGC, GCForeground|GCFont, &gcv);
  
  gcv.foreground = getMResource()->frame.textColor.pixel;
  gcv.font = getMenuFont()->fid;
  XChangeGC(display, mframeGC, GCForeground|GCFont, &gcv);

  gcv.foreground = getMResource()->frame.hiTextColor.pixel;
  XChangeGC(display, mhiGC, GCForeground|GCBackground|GCFont, &gcv);

  gcv.foreground = getMResource()->frame.hiColor.pixel;
  gcv.arc_mode = ArcChord;
  gcv.fill_style = FillSolid;
  XChangeGC(display, mhbgGC, GCForeground|GCFillStyle|GCArcMode, &gcv);
  
  InitMenu();
   
  workspacemenu->reconfigure();
  iconmenu->reconfigure();
  rootmenu->reconfigure();
  toolbar->reconfigure();

  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it++)
    it.current()->reconfigure();
}


void BScreen::removeWorkspaceNames(void) {
  while (workspaceNames->count())
    workspaceNames->remove(0);
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
  if (! resource.wres.decoration.focusTextColor.allocated)
    resource.wres.decoration.focusTextColor.pixel =
      image_control->getColor("white",
			      &resource.wres.decoration.focusTextColor.red,
			      &resource.wres.decoration.focusTextColor.green,
			      &resource.wres.decoration.focusTextColor.blue);
  readDatabaseColor("window.unfocus.textColor", "Window.Unfocus.TextColor",
		    &(resource.wres.decoration.unfocusTextColor));
  if (! resource.wres.decoration.unfocusTextColor.allocated)
    resource.wres.decoration.unfocusTextColor.pixel =
      image_control->getColor("darkgrey",
			      &resource.wres.decoration.unfocusTextColor.red,
			      &resource.wres.decoration.unfocusTextColor.green,
			      &resource.wres.decoration.unfocusTextColor.blue);
  
  // load menu configuration
  readDatabaseTexture("menu.title", "Menu.Title",
		      &(resource.mres.title.texture));
  readDatabaseTexture("menu.frame", "Menu.Frame",
		      &(resource.mres.frame.texture));
  
  readDatabaseColor("menu.title.textColor", "Menu.Title.TextColor",
		    &(resource.mres.title.textColor));
  if (! resource.mres.title.textColor.allocated)
    resource.mres.title.textColor.pixel =
      image_control->getColor("white", &resource.mres.title.textColor.red,
			      &resource.mres.title.textColor.green,
			      &resource.mres.title.textColor.blue);
  
  readDatabaseColor("menu.frame.highlightColor",
			   "Menu.Frame.HighLightColor",
			   &(resource.mres.frame.hiColor));
  if (! resource.mres.frame.hiColor.allocated)
    resource.mres.frame.hiColor.pixel =
      image_control->getColor("black",
			      &resource.mres.frame.hiColor.red,
			      &resource.mres.frame.hiColor.green,
			      &resource.mres.frame.hiColor.blue);

  readDatabaseColor("menu.frame.textColor", "Menu.Frame.TextColor",
		    &(resource.mres.frame.textColor));
  if (! resource.mres.frame.textColor.allocated)
    resource.mres.frame.textColor.pixel =
      image_control->getColor("black",
			      &resource.mres.frame.textColor.red,
			      &resource.mres.frame.textColor.green,
			      &resource.mres.frame.textColor.blue);
  
  readDatabaseColor("menu.frame.hiTextColor", "Menu.Frame.HiTextColor",
		    &(resource.mres.frame.hiTextColor));
  if (! resource.mres.frame.hiTextColor.allocated)
    resource.mres.frame.hiTextColor.pixel =
      image_control->getColor("white",
			      &resource.mres.frame.hiTextColor.red,
			      &resource.mres.frame.hiTextColor.green,
			      &resource.mres.frame.hiTextColor.blue);

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
  if (! resource.tres.toolbar.textColor.allocated)
    resource.tres.toolbar.textColor.pixel =
      image_control->getColor("black",
			      &resource.tres.toolbar.textColor.red,
			      &resource.tres.toolbar.textColor.green,
			      &resource.tres.toolbar.textColor.blue);

  // load border color
  readDatabaseColor("borderColor", "BorderColor", &(resource.border_color));
  if (! resource.border_color.allocated)
    resource.border_color.pixel =
      image_control->getColor("black",
			      &resource.border_color.red,
			      &resource.border_color.green,
			      &resource.border_color.blue);
  
  // load border and handle widths
  if (XrmGetResource(resource.stylerc,
		     "handleWidth",
		     "HandleWidth", &value_type,
		     &value)) {
    if (sscanf(value.addr, "%u", &resource.handle_width) != 1)
      resource.handle_width = 8;
    else
      if (resource.handle_width > (xres / 2) ||
          resource.handle_width == 0)
	resource.handle_width = 8;
  } else
    resource.handle_width = 8;

  if (XrmGetResource(resource.stylerc,
		     "bevelWidth",
		     "BevelWidth", &value_type,
		     &value)) {
    if (sscanf(value.addr, "%u", &resource.bevel_width) != 1)
      resource.bevel_width = 4;
    else
      if (resource.bevel_width > (xres / 2) || resource.bevel_width == 0)
	resource.bevel_width = 4;
  } else
    resource.bevel_width = 4;

  if (XrmGetResource(resource.stylerc,
		     "titleJustify",
		     "TitleJustify", &value_type, &value)) {
    if (! strncasecmp("leftjustify", value.addr, value.size))
      resource.justify = Blackbox::B_LeftJustify;
    else if (! strncasecmp("rightjustify", value.addr, value.size))
      resource.justify = Blackbox::B_RightJustify;
    else if (! strncasecmp("centerjustify", value.addr, value.size))
      resource.justify = Blackbox::B_CenterJustify;
    else
      resource.justify = Blackbox::B_LeftJustify;
  } else
    resource.justify = Blackbox::B_LeftJustify;

  if (XrmGetResource(resource.stylerc,
		     "menuJustify",
		     "MenuJustify", &value_type, &value)) {
    if (! strncasecmp("leftjustify", value.addr, value.size))
      resource.menu_justify = Blackbox::B_LeftJustify;
    else if (! strncasecmp("rightjustify", value.addr, value.size))
      resource.menu_justify = Blackbox::B_RightJustify;
    else if (! strncasecmp("centerjustify", value.addr, value.size))
      resource.menu_justify = Blackbox::B_CenterJustify;
    else
      resource.menu_justify = Blackbox::B_LeftJustify;
  } else
    resource.menu_justify = Blackbox::B_LeftJustify;

  if (XrmGetResource(resource.stylerc,
		     "moveStyle",
		     "MoveStyle", &value_type, &value)) {
    if (! strncasecmp("opaque", value.addr, value.size))
      resource.opaque_move = True;
    else
      resource.opaque_move = False;
  } else
    resource.opaque_move = False;

  const char *defaultFont = "fixed";
  if (resource.font.title) {
    XFreeFont(display, resource.font.title);
    resource.font.title = 0;
  }

  if (XrmGetResource(resource.stylerc,
		     "titleFont",
		     "TitleFont", &value_type, &value)) {
    if ((resource.font.title = XLoadQueryFont(display, value.addr)) == NULL) {
      fprintf(stderr,
	      "blackbox: couldn't load font '%s'\n"
              "          reverting to default font.\n", value.addr);
      if ((resource.font.title = XLoadQueryFont(display, defaultFont))
	  == NULL) {
	fprintf(stderr,
		"blackbox: couldn't load default font.  please check to\n"
		"make sure the necessary font is installed '%s'\n",
		defaultFont);
	exit(2);
      }  
    }
  } else {
    if ((resource.font.title = XLoadQueryFont(display, defaultFont)) == NULL) {
      fprintf(stderr,
	      "blackbox: couldn't load default font.  please check to\n"
	      "make sure the necessary font is installed '%s'\n", defaultFont);
      exit(2);
    }
  }

  if (resource.font.menu) {
    XFreeFont(display, resource.font.menu);
    resource.font.menu = 0;
  }

  if (XrmGetResource(resource.stylerc,
		     "menuFont",
		     "MenuFont", &value_type, &value)) {
    if ((resource.font.menu = XLoadQueryFont(display, value.addr)) == NULL) {
      fprintf(stderr,
	      "blackbox: couldn't load font '%s'\n"
	      "          reverting to default font.\n", value.addr);
      if ((resource.font.menu = XLoadQueryFont(display, defaultFont))
	  == NULL) {
	fprintf(stderr,
		"blackbox: couldn't load default font.  please check to\n"
		"make sure the necessary font is installed '%s'\n",
		defaultFont);
	exit(2);
      }  
    }
  } else {
    if ((resource.font.menu = XLoadQueryFont(display, defaultFont)) == NULL) {
      fprintf(stderr,
	      "blackbox: couldn't load default font.  please check to\n"
	      "make sure the necessary font is installed '%s'\n", defaultFont);
      exit(2);
    }
  }

  if (XrmGetResource(resource.stylerc,
                     "rootCommand",
                     "RootCommand", &value_type, &value)) {
#ifndef __EMX__
    int dslen = strlen(DisplayString(display));
    
    char *displaystring = new char[dslen + 32];
    char *command = new char[strlen(value.addr) + dslen + 64];
    
    sprintf(displaystring, "%s", DisplayString(display));
    // gotta love pointer math
    sprintf(displaystring + dslen - 1, "%d", screen_number);
    sprintf(command, "DISPLAY=%s exec %s &", displaystring, value.addr);
    system(command);
    
    delete [] displaystring;
    delete [] command;
#else
    spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", item->exec(), NULL);
#endif
  }
  
  XrmDestroyDatabase(resource.stylerc);
}


int BScreen::addWorkspace(void) {
  Workspace *wkspc = new Workspace(this, workspacesList->count());
  workspacesList->insert(wkspc);

  workspacemenu->insert(wkspc->getName(), wkspc->getMenu(),
			wkspc->getWorkspaceID() + 1);
  workspacemenu->update();
  if (workspacemenu->isVisible())
    workspacemenu->move(toolbar->getX(), toolbar->getY() - 
			workspacemenu->getHeight() - 1);

#ifdef    KDE
  unsigned long data = (unsigned long) workspacesList->count();
  
  XChangeProperty(display, root_window,
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
    if (workspacemenu->isVisible())
      workspacemenu->move(toolbar->getX(), toolbar->getY() -
			  workspacemenu->getHeight() - 1);

    workspacesList->remove(wkspc);
    delete wkspc;
    
#ifdef    KDE
    unsigned long data = (unsigned long) workspacesList->count();
    
    XChangeProperty(display, root_window,
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
   
    if (workspacemenu->isVisible()) {
      workspacemenu->hide();
      toolbar->redrawMenuButton(False, True);
    }

    toolbar->redrawWorkspaceLabel(True);
    workspacemenu->setHighlight(id + 2);
    
    current_workspace->showAll();
  }

#ifdef    KDE
  unsigned long data = (unsigned long) current_workspace->getWorkspaceID() + 1;
  
  XChangeProperty(display, root_window, blackbox->getKWMCurrentDesktopAtom(),
                  blackbox->getKWMCurrentDesktopAtom(), 32, PropModeReplace,
                  (unsigned char *) &data, 1);
  
  sendToKWMModules(blackbox->getKWMModuleDesktopChangeAtom(), (XID) data);
#endif // KDE
}


void BScreen::raiseWindows(Window *workspace_stack, int num) {
  Window *session_stack = new Window[(num + workspacesList->count() + 4)];

  blackbox->grab();
  
  int i = 0, k = 0;

  XRaiseWindow(display, rootmenu->getWindowID());
  *(session_stack + i++) = rootmenu->getWindowID();
  
  if (toolbar->isOnTop()) {
    *(session_stack + i++) = iconmenu->getWindowID();
    *(session_stack + i++) = workspacemenu->getWindowID();
    
    LinkedListIterator<Workspace> it(workspacesList);
    for (; it.current(); it++)
      *(session_stack + i++) = it.current()->getMenu()->getWindowID();
    
    *(session_stack + i++) = toolbar->getWindowID();  
  }
  
  k = num;
  while (k--)
    *(session_stack + i++) = *(workspace_stack + k);
  
  XRestackWindows(display, session_stack, i);
  
  blackbox->ungrab();
  
  delete [] session_stack;
}


#ifdef HAVE_STRFTIME
void BScreen::saveStrftimeFormat(char *format) {
  if (resource.strftime_format) delete [] resource.strftime_format;
  
  resource.strftime_format = new char[strlen(format) + 1];
  sprintf(resource.strftime_format, "%s", format);
}
#endif


void BScreen::addWorkspaceName(char *name) {
  workspaceNames->insert(name);
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
	screen_number) {
      have_focused = True;
      focused_window_number = blackbox->getFocusedWindow()->getWindowNumber();
    }
  
  if ((getCurrentWorkspace()->getCount() > 1) && have_focused) {
    BlackboxWindow *next;
    
    int next_window_number = focused_window_number;
    do {
      do {
	if ((++next_window_number) >= getCurrentWorkspace()->getCount()) {
	  next_window_number = 0;
	}
	
	next = getCurrentWorkspace()->getWindow(next_window_number);
 
      }	while (next->isIconic() && (next_window_number !=
				    focused_window_number));
    } while ((! next->setInputFocus()) && (next_window_number !=
					   focused_window_number));
    
    if (next_window_number != focused_window_number) {
      blackbox->getFocusedWindow()->getScreen()->
	getWorkspace(blackbox->getFocusedWindow()->getWorkspaceNumber())->
	setFocusWindow(-1);
      blackbox->getFocusedWindow()->setFocusFlag(False);

      getCurrentWorkspace()->raiseWindow(next);
    }
  } else if (getCurrentWorkspace()->getCount() >= 1) {
    getCurrentWorkspace()->getWindow(0)->setInputFocus();
  }
}


void BScreen::prevFocus(void) {
  Bool have_focused = False;
  int focused_window_number = -1;
  
  if (blackbox->getFocusedWindow())
    if (blackbox->getFocusedWindow()->getScreen()->getScreenNumber() ==
	screen_number) {
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
    
    if (prev_window_number != focused_window_number) {
      blackbox->getFocusedWindow()->getScreen()->
	getWorkspace(blackbox->getFocusedWindow()->getWorkspaceNumber())->
	setFocusWindow(-1);
      blackbox->getFocusedWindow()->setFocusFlag(False);

      getCurrentWorkspace()->raiseWindow(prev);
    }
  } else if (getCurrentWorkspace()->getCount() >= 1)
    getCurrentWorkspace()->getWindow(0)->setInputFocus();
}


void BScreen::raiseFocus(void) {
  Bool have_focused = False;
  int focused_window_number = -1;

  if (blackbox->getFocusedWindow())
    if (blackbox->getFocusedWindow()->getScreen()->getScreenNumber() ==
	screen_number) {
      have_focused = True;
      focused_window_number = blackbox->getFocusedWindow()->getWindowNumber();
    }
  
  if ((getCurrentWorkspace()->getCount() > 1) && have_focused)
    getWorkspace(blackbox->getFocusedWindow()->getWorkspaceNumber())->
      raiseWindow(blackbox->getFocusedWindow()); 
}


void BScreen::InitMenu(void) {
  if (rootmenu) {
    int i, n = rootmenu->getCount();
    for (i = 0; i < n; i++)
      rootmenu->remove(0);
  } else
    rootmenu = new Rootmenu(blackbox, this);
  
  Bool defaultMenu = True;

  if (blackbox->getMenuFilename()) {
    FILE *menu_file = fopen(blackbox->getMenuFilename(), "r");

    if (menu_file) {
      if (! feof(menu_file)) {
	char line[1024], tmp1[1024];
	memset(line, 0, 1024);
	memset(tmp1, 0, 1024);

	while (fgets(line, 1024, menu_file) && ! feof(menu_file)) {
	  if (line[0] != '#') {
	    int i, ri, len = strlen(line);
	    
	    for (i = 0; i < len; i++)
	      if (line[i] == '[') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ']') break;
	    
	    if (i < ri && ri > 0) {
	      strncpy(tmp1, line + i, ri - i);
	      *(tmp1 + (ri - i)) = '\0';
	      
	      if (strstr(tmp1, "begin")) {
		for (i = 0; i < len; i++)
		  if (line[i] == '(') { i++; break; }
		for (ri = len; i < len; ri--)
		  if (line[ri] == ')') { break; }
		
		char label[1024];
		if (i < ri && ri > 0) {
		  strncpy(label, line + i, ri - i);
		  *(label + (ri - i)) = '\0';
		} else
                  label[0] = '\0';
		
		rootmenu->setMenuLabel(label);
		defaultMenu = parseMenuFile(menu_file, rootmenu);
		break;
	      }
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
    rootmenu->insert("xterm", Blackbox::B_Execute, "xterm");
    rootmenu->insert("Restart", Blackbox::B_Restart);
    rootmenu->insert("Exit", Blackbox::B_Exit);
  }
}


Bool BScreen::parseMenuFile(FILE *file, Rootmenu *menu) {
  char line[512], tmp1[512], tmp2[512];

  while (! feof(file)) {
    memset(line, 0, 512);
    memset(tmp1, 0, 512);
    memset(tmp2, 0, 512);
    
    if (fgets(line, 512, file)) {
      if (line[0] != '#') {
	int i, ri, len = strlen(line);

	for (i = 0; i < len; i++)
	  if (line[i] == '[') { i++; break; }
	for (ri = len; ri > 0; ri--)
	  if (line[ri] == ']') break;

	if (i < ri && ri > 0) {
	  strncpy(tmp1, line + i, ri - i);
	  *(tmp1 + (ri - i)) = '\0';
	  
	  if (strstr(tmp1, "nop")) {
            for (i = 0; i < len; i++)
              if (line[i] == '(') { i++; break; }
            for (ri = len; ri > 0; ri--)
              if (line[ri] == ')') break;

	    char *label = 0;
            if (i < ri && ri > 0) {
              label = new char[ri - i + 1];
              strncpy(label, line + i, ri - i);
              *(label + (ri - i)) = '\0';

              menu->insert(label, 0);
            } else {
	      label = new char[1];
	      *label = '\0';
	      menu->insert(label);
	    }
	  } else if (strstr(tmp1, "exit")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;
	    
	    if (i < ri && ri > 0) {
	      char *label = new char[ri - i + 1];
	      strncpy(label, line + i, ri - i);
	      *(label + (ri - i)) = '\0';
	      
	      menu->insert(label, Blackbox::B_Exit);
	    }
	  } else if (strstr(tmp1, "restart")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;
	    
	    if (i < ri && ri > 0) {
	      char *label = new char[ri - i + 1];
	      strncpy(label, line + i, ri - i);
	      *(label + (ri - i)) = '\0';
	      
	      for (i = 0; i < len; i++)
		if (line[i] == '{') { i++; break; }
	      for (ri = len; ri > 0; ri--)
		if (line[ri] == '}') break;
	      
	      char *other = 0;
	      if (i < ri && ri > 0) {
		other = new char[ri - i + 1];
		strncpy(other, line + i, ri - i);
		*(other + (ri - i)) = '\0';
	      }

	      if (other)
		menu->insert(label, Blackbox::B_RestartOther, other);
	      else
		menu->insert(label, Blackbox::B_Restart);
	    }
	  } else if (strstr(tmp1, "reconfig")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;
	    
	    if (i < ri && ri > 0) {
	      char *label = new char[ri - i + 1];
	      strncpy(label, line + i, ri - i);
	      *(label + (ri - i)) = '\0';
	      
	      for (i = 0; i < len; i++)
		if (line[i] == '{') { i++; break; }
	      for (ri = len; ri > 0; ri--)
		if (line[ri] == '}') break;
	      
	      char *exec = 0;
	      if (i < ri && ri > 0) {
		exec = new char[ri - i + 1];
		strncpy(exec, line + i, ri - i);
		*(exec + (ri - i)) = '\0';
	      }

	      if (exec)
		menu->insert(label, Blackbox::B_ExecReconfigure, exec);
	      else
		menu->insert(label, Blackbox::B_Reconfigure);
	    }
	  } else if (strstr(tmp1, "submenu")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;

	    char *label = 0;
	    if (i < ri && ri > 0) {
	      label = new char[ri - i + 1];
	      strncpy(label, line + i, ri - i);
	      *(label + (ri - i)) = '\0';
	    }
	    
	    // this is an optional feature
	    for (i = 0; i < len; i++)
	      if (line[i] == '{') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == '}') break;
	    
	    char title[512];
	    if (i < ri && ri > 0) {
	      strncpy(title, line + i, ri - i);
	      *(title + (ri - i)) = '\0';
	    } else {
	      int l = strlen(label);
	      strncpy(title, label, l + 1);
	    }
	    
	    Rootmenu *submenu = new Rootmenu(blackbox, this);
	    submenu->setMenuLabel(title);
	    parseMenuFile(file, submenu);
	    submenu->update();
	    menu->insert(label, submenu);
	  } else if (strstr(tmp1, "end")) {
	    break;
	  } else if (strstr(tmp1, "exec")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;

	    if (i < ri && ri > 0) {
	      char *label = new char[ri - i + 1];
	      strncpy(label, line + i, ri - i);
	      *(label + (ri - i)) = '\0';
   
	      for (i = 0; i < len; i++)
		if (line[i] == '{') { i++; break; }
	      for (ri = len; ri > 0; ri--)
		if (line[ri] == '}') break;
	      
	      if (i < ri && ri > 0) {
		char *command = new char[ri - i + 1];
		strncpy(command, line + i, ri - i);
		*(command + (ri - i)) = '\0';

		if (label && command)
		  menu->insert(label, Blackbox::B_Execute, command);
		else
		  fprintf(stderr, "BScreen::parseMenuFile: "
			  "[exec] error: label(%s) == NULL"
			  "|| command(%s) == NULL\n", label, command);
	      } else
		fprintf(stderr, "BScreen::parseMenuFile: "
			"[exec] error: no command string (%s)\n", label);
	    } else
	      fprintf(stderr, "BScreen::parseMenuFile: "
		      "[exec] error: no label string\n");
	  } else if (strstr(tmp1, "include")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;

	    if (i < ri && ri > 0) {
	      char *newfile = new char[ri - i + 1];
	      strncpy(newfile, line + i, ri - i);
	      *(newfile + (ri - i)) = '\0';
	      
	      if (newfile) {
		FILE *submenufile = fopen(newfile, "r");
		
		if (submenufile) {
		  if (! feof(submenufile)) {
		    parseMenuFile(submenufile, menu);
		    fclose(submenufile);
		  }
		} else
		  perror(newfile);

		delete [] newfile;
	      } else
		fprintf(stderr, "BScreen::parseMenuFile: "
			"[include] error: newfile(%s) == NULL\n", newfile);
	    }
	  } else if (strstr(tmp1, "style")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;
	    
	    if (i < ri && ri > 0) {
	      char *label = new char[ri - i + 1];
	      strncpy(label, line + i, ri - i);
	      *(label + (ri - i)) = '\0';
	      
	      for (i = 0; i < len; i++)
		if (line[i] == '{') { i++; break; }
	      for (ri = len; ri > 0; ri--)
		if (line[ri] == '}') break;
	      
	      if (i < ri && ri > 0) {
		char *style = new char[ri - i + 1];
		strncpy(style, line + i, ri - i);
		*(style + (ri - i)) = '\0';

		if (label && style)
		  menu->insert(label, Blackbox::B_SetStyle, style);
		else
		  fprintf(stderr, "BScreen::parseMenuFile: "
			  "[style] error: label(%s) == NULL || "
			  "style(%s) == NULL\n", label, style);
	      } else
		fprintf(stderr, "BScreen::parseMenuFile: "
			"[style] error: no style filename (%s)\n", label);
	    } else
	      fprintf(stderr, "BScreen::parseMenuFile: "
		      "[style] error: no label string\n");
          }
	}
      }
    }
  }
  
  return ((menu->getCount() == 0) ? True : False);
}


void BScreen::shutdown(void) {
  blackbox->grab();

  XSelectInput(display, root_window, NoEventMask);
  XSync(display, False);

#ifdef    KDE
  XDeleteProperty(display, root_window, blackbox->getKWMRunningAtom());
#endif // KDE

  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it ++)
    it.current()->shutdown();
  
  blackbox->ungrab();
}


#ifdef    KDE

Bool BScreen::isKWMModule(Window win) {
  Atom type_return;

  int ijunk;
  unsigned long uljunk, return_value = 0, *result;

  if (XGetWindowProperty(display, win, blackbox->getKWMModuleAtom(), 0l, 1l,
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
    XSendEvent(display, *(it.current()), False,
               0 | ((*(it.current()) == root_window) ?
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
  
  mask = 0 | ((window == root_window) ? SubstructureRedirectMask : 0);
  XSendEvent(display, window, False, mask, &e);
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

    XSelectInput(display, win, StructureNotifyMask);
  } else
    removeKWMWindow(win);
}


void BScreen::scanWorkspaceNames(void) {
  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it++)
    it.current()->rereadName();
}

#endif // KDE

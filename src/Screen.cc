// Screen.cc for Blackbox - an X11 Window manager
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
  
// stupid macros needed to access some functions in version 2 of the GNU C 
// library
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

#ifdef    DEBUG
#  include "mem.h"
#endif // DEBUG

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
#  include <sys/types.h>
#endif // STDC_HEADERS

#ifdef    HAVE_CTYPE_H
#  include <ctype.h>
#endif // HAVE_CTYPE_H

#ifdef    HAVE_DIRENT_H
#  include <dirent.h>
#endif // HAVE_DIRENT_H

#ifdef    HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

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

static int dcmp(const void *one, const void *two) {
  return (strcmp((*(char **) one), (*(char **) two)));
}


BScreen::BScreen(Blackbox *bb, int scrn) : ScreenInfo(bb, scrn) {
#ifdef    DEBUG
  allocate(sizeof(BScreen), "Screen.cc");
#endif // DEBUG

  fprintf(stderr, "BScreen::BScreen: using visual 0x%lx, depth %d\n",
         XVisualIDFromVisual(getVisual()), getDepth());

  blackbox = bb;

  event_mask = ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
    SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
    ButtonPressMask | ButtonReleaseMask;
  
  XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(getBaseDisplay()->getXDisplay(), getRootWindow(), event_mask);
  XSync(getBaseDisplay()->getXDisplay(), False);
  XSetErrorHandler((XErrorHandler) old);

  managed = running;
  if (! managed) return;

  
  
  rootmenu = 0;
  resource.stylerc = 0;
  resource.mstyle.t_font = resource.mstyle.f_font = resource.tstyle.font =
    resource.wstyle.font = (XFontStruct *) 0;
 
#ifdef    HAVE_STRFTIME
  resource.strftime_format = 0;
#endif // HAVE_STRFTIME

#ifdef    HAVE_GETPID
  pid_t bpid = getpid();

  XChangeProperty(getBaseDisplay()->getXDisplay(), getRootWindow(),
                  blackbox->getBlackboxPidAtom(), XA_CARDINAL,
                  sizeof(pid_t) * 8, PropModeReplace,
                  (unsigned char *) &bpid, 1); 
#endif // HAVE_GETPID

  XDefineCursor(getBaseDisplay()->getXDisplay(), getRootWindow(),
                blackbox->getSessionCursor());

  workspaceNames = new LinkedList<char>;
  workspacesList = new LinkedList<Workspace>;
  
  rootmenuList = new LinkedList<Rootmenu>;
  netizenList = new LinkedList<Netizen>;
  
  image_control =
    new BImageControl(blackbox, this, True, blackbox->getColorsPerChannel(),
                      blackbox->getCacheLife(), blackbox->getCacheMax());
  image_control->installRootColormap();
  root_colormap_installed = True;
  
  blackbox->load_rc(this);
  
  image_control->setDither(resource.image_dither);

  LoadStyle();
  
  XGCValues gcv;
  gcv.foreground = WhitePixel(getBaseDisplay()->getXDisplay(),
			      getScreenNumber());
  gcv.function = GXinvert;
  gcv.subwindow_mode = IncludeInferiors;
  opGC = XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
                   GCForeground | GCFunction | GCSubwindowMode, &gcv);

  gcv.foreground = resource.wstyle.l_text_focus.getPixel();
  gcv.font = resource.wstyle.font->fid;
  resource.wstyle.l_text_focus_gc =
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground | GCFont, &gcv);
  
  gcv.foreground = resource.wstyle.l_text_unfocus.getPixel();
  gcv.font = resource.wstyle.font->fid;
  resource.wstyle.l_text_unfocus_gc =
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground | GCFont, &gcv);

  gcv.foreground = resource.wstyle.b_pic_focus.getPixel();
  resource.wstyle.b_pic_focus_gc =
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground, &gcv);

  gcv.foreground = resource.wstyle.b_pic_unfocus.getPixel();
  resource.wstyle.b_pic_unfocus_gc =
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground, &gcv);

  gcv.foreground = resource.mstyle.t_text.getPixel();
  gcv.font = resource.mstyle.t_font->fid;
  resource.mstyle.t_text_gc =
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground | GCFont, &gcv);

  gcv.foreground = resource.mstyle.f_text.getPixel();
  gcv.font = resource.mstyle.f_font->fid;
  resource.mstyle.f_text_gc =
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground | GCFont, &gcv);

  gcv.foreground = resource.mstyle.h_text.getPixel();
  resource.mstyle.h_text_gc =
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground | GCFont, &gcv);
  
  gcv.foreground = resource.mstyle.d_text.getPixel();
  resource.mstyle.d_text_gc =
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground | GCFont, &gcv);
  
  gcv.foreground = resource.tstyle.l_text.getPixel();
  gcv.font = resource.tstyle.font->fid;
  resource.tstyle.l_text_gc = 
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground | GCFont, &gcv);

  gcv.foreground = resource.tstyle.w_text.getPixel();
  resource.tstyle.w_text_gc = 
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground | GCFont, &gcv);
  
  gcv.foreground = resource.tstyle.c_text.getPixel();
  resource.tstyle.c_text_gc =
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground | GCFont, &gcv);

  gcv.foreground = resource.tstyle.b_pic.getPixel();
  resource.tstyle.b_pic_gc =
    XCreateGC(getBaseDisplay()->getXDisplay(), getRootWindow(),
	      GCForeground | GCFont, &gcv);
  
  geom_h = resource.wstyle.font->ascent +
    resource.wstyle.font->descent +
    (resource.bevel_width * 2);
  geom_w = (resource.bevel_width * 2) +
    XTextWidth(resource.wstyle.font, "0: 0000 x 0: 0000",
	       strlen("0: 0000 x 0: 0000"));
  
  geom_pixmap =
    image_control->renderImage(geom_w, geom_h, &resource.wstyle.l_focus);
  
  XSetWindowAttributes attrib;
  unsigned long mask = CWBackPixmap | CWBorderPixel | CWSaveUnder;
  attrib.background_pixmap = geom_pixmap;
  attrib.border_pixel = getBorderColor()->getPixel();
  attrib.save_under = True;

  geom_window =
    XCreateWindow(getBaseDisplay()->getXDisplay(), getRootWindow(),
                  0, 0, geom_w, geom_h, resource.border_width, getDepth(),
                  InputOutput, getVisual(), mask, &attrib);
  geom_visible = False;
  
  workspacemenu = new Workspacemenu(this);
  iconmenu = new Iconmenu(this);
  configmenu = new Configmenu(this);
  
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
  
  workspacemenu->insert("Icons", iconmenu);
  workspacemenu->update();
  
  current_workspace = workspacesList->first();
  workspacemenu->setItemSelected(2, True);
  
  toolbar = new Toolbar(this);

#ifdef    SLIT
  slit = new Slit(this);
#endif // SLIT
  
  InitMenu();
  
  raiseWindows(0, 0);
  rootmenu->update();

  changeWorkspaceID(0);

  int i;
  unsigned int nchild;
  Window r, p, *children;
  XQueryTree(getBaseDisplay()->getXDisplay(), getRootWindow(), &r, &p,
	     &children, &nchild);

  // preen the window list of all icon windows... for better dockapp support
  for (i = 0; i < (int) nchild; i++) {
    if (children[i] == None) continue;

    XWMHints *wmhints = XGetWMHints(getBaseDisplay()->getXDisplay(),
				    children[i]);
    
    if (wmhints) {
      if ((wmhints->flags & IconWindowHint) &&
	  (wmhints->icon_window != children[i]))
        for (int j = 0; j < (int) nchild; j++)
          if (children[j] == wmhints->icon_window) {
            children[j] = None;
	    
            break;
          }
      
      XFree(wmhints);
    }
  }
 
  // manage shown windows
  for (i = 0; i < (int) nchild; ++i) {
    if (children[i] == None || (! blackbox->validateWindow(children[i])))
      continue;
    
    XWindowAttributes attrib;
    if (XGetWindowAttributes(getBaseDisplay()->getXDisplay(), children[i],
                             &attrib)) {
      if (attrib.override_redirect) continue;
    
      if (attrib.map_state != IsUnmapped) {
        new BlackboxWindow(blackbox, children[i], this);
        
        BlackboxWindow *win = blackbox->searchWindow(children[i]);
        if (win) {
          XMapRequestEvent mre;
          mre.window = children[i];
          win->restoreAttributes();
	  win->mapRequestEvent(&mre);
        }
      }
    }
  }

  if (! resource.sloppy_focus)
    XSetInputFocus(getBaseDisplay()->getXDisplay(), toolbar->getWindowID(),
		   RevertToParent, CurrentTime);
  
  XFree(children);
  XFlush(getBaseDisplay()->getXDisplay());
}


BScreen::~BScreen(void) {
#ifdef    DEBUG
  deallocate(sizeof(BScreen), "Screen.cc");
#endif // DEBUG

  if (! managed) return;

  if (geom_pixmap != None)
    image_control->removeImage(geom_pixmap);    

  if (geom_window != None)
    XDestroyWindow(getBaseDisplay()->getXDisplay(), geom_window);

  removeWorkspaceNames();

  while (workspacesList->count())
    delete workspacesList->remove(0);
  
  while (rootmenuList->count())
    rootmenuList->remove(0);
  
#ifdef    HAVE_STRFTIME
  if (resource.strftime_format) {
#  ifdef    DEBUG
    deallocate(sizeof(char) * (strlen(resource.strftime_format) + 1), "Screen.cc");
#  endif // DEBUG
    delete [] resource.strftime_format;
  }
#endif // HAVE_STRFTIME

  delete rootmenu;
  delete workspacemenu;
  delete iconmenu;
  delete configmenu;

#ifdef    SLIT
  delete slit;
#endif // SLIT

  delete toolbar;
  delete image_control;

  delete workspacesList;
  delete workspaceNames;

  delete rootmenuList;

  if (resource.wstyle.font)
    XFreeFont(getBaseDisplay()->getXDisplay(), resource.wstyle.font);
  
  if (resource.mstyle.t_font)
    XFreeFont(getBaseDisplay()->getXDisplay(), resource.mstyle.t_font);

  if (resource.mstyle.f_font)
    XFreeFont(getBaseDisplay()->getXDisplay(), resource.mstyle.f_font);

  XFreeGC(getBaseDisplay()->getXDisplay(), opGC);

  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.wstyle.l_text_focus_gc);
  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.wstyle.l_text_unfocus_gc);
  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.wstyle.b_pic_focus_gc);
  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.wstyle.b_pic_unfocus_gc);

  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.mstyle.t_text_gc);
  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.mstyle.f_text_gc);
  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.mstyle.h_text_gc);
  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.mstyle.d_text_gc);
  
  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.tstyle.l_text_gc);
  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.tstyle.w_text_gc);
  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.tstyle.c_text_gc);
  XFreeGC(getBaseDisplay()->getXDisplay(),
	  resource.tstyle.b_pic_gc);
}

void BScreen::readDatabaseTexture(char *rname, char *rclass,
				  BTexture *texture,
				  unsigned long default_pixel)
{
  XrmValue value;
  char *value_type;
  
  if (XrmGetResource(resource.stylerc, rname, rclass, &value_type,
		     &value))
    image_control->parseTexture(texture, value.addr);
  else
    texture->setTexture(BImage_Solid | BImage_Flat);
  
  if (texture->getTexture() & BImage_Solid) {
    int clen = strlen(rclass) + 8, nlen = strlen(rname) + 8;

#ifdef    DEBUG
    allocate(sizeof(char) * (clen + nlen), "Screen.cc");
#endif // DEBUG

    char *colorclass = new char[clen], *colorname = new char[nlen];
    
    sprintf(colorclass, "%s.Color", rclass);
    sprintf(colorname,  "%s.color", rname);
    
    readDatabaseColor(colorname, colorclass, texture->getColor(),
		      default_pixel);

#ifdef    INTERLACED
    sprintf(colorclass, "%s.ColorTo", rclass);
    sprintf(colorname,  "%s.colorTo," rname);

    readDatabaseColor(colorname, colorclass, texture->getcolor(),
                      default_pixel);
#endif // INTERLACED
    
#ifdef    DEBUG
    deallocate(sizeof(char) * (clen + nlen), "Screen.cc");
#endif // DEBUG

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
    
    if (! XAllocColor(getBaseDisplay()->getXDisplay(),
		      image_control->getColormap(), &xcol))
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
    
    if (! XAllocColor(getBaseDisplay()->getXDisplay(),
		      image_control->getColormap(), &xcol))
      xcol.pixel = 0;
    
    texture->getLoColor()->setPixel(xcol.pixel);
  } else if (texture->getTexture() & BImage_Gradient) {
    int clen = strlen(rclass) + 10, nlen = strlen(rname) + 10;

#ifdef    DEBUG
    allocate(sizeof(char) * 2 * (clen + nlen), "Screen.cc");
#endif // DEBUG

    char *colorclass = new char[clen], *colorname = new char[nlen],
      *colortoclass = new char[clen], *colortoname = new char[nlen];
    
    sprintf(colorclass, "%s.Color", rclass);
    sprintf(colorname,  "%s.color", rname);
    
    sprintf(colortoclass, "%s.ColorTo", rclass);
    sprintf(colortoname,  "%s.colorTo", rname);
    
    readDatabaseColor(colorname, colorclass, texture->getColor(),
		      default_pixel);
    readDatabaseColor(colortoname, colortoclass, texture->getColorTo(),
		      default_pixel);
    
#ifdef    DEBUG
    deallocate(sizeof(char) * 2 * (clen + nlen), "Screen.cc");
#endif // DEBUG

    delete [] colorclass;
    delete [] colorname;
    delete [] colortoclass;
    delete [] colortoname;
  }
}


void BScreen::readDatabaseColor(char *rname, char *rclass, BColor *color,
				unsigned long default_pixel)
{
  XrmValue value;
  char *value_type;
  
  if (XrmGetResource(resource.stylerc, rname, rclass, &value_type,
		     &value)) {
    image_control->parseColor(color, value.addr);
  } else {
    // parsing with no color string just deallocates the color, if it has
    // been previously allocated
    image_control->parseColor(color);
    color->setPixel(default_pixel);
  }
}


void BScreen::readDatabaseFont(char *rname, char *rclass, XFontStruct **font) {
  if (! font) return;
  
  static const char *defaultFont = "fixed";
  
  Bool load_default = False;
  XrmValue value;
  char *value_type;
  
  if (*font)
    XFreeFont(getBaseDisplay()->getXDisplay(), *font);
  
  if (XrmGetResource(resource.stylerc, rname, rclass, &value_type, &value)) {
    if ((*font = XLoadQueryFont(getBaseDisplay()->getXDisplay(),
				value.addr)) == NULL) {
      fprintf(stderr,
	      "BScreen::LoadStyle(): couldn't load font '%s'\n", value.addr);
      
      load_default = True;
    }
  } else
    load_default = True;

  if (load_default && (*font = XLoadQueryFont(getBaseDisplay()->getXDisplay(),
                                              defaultFont)) == NULL) {
    fprintf(stderr, "BScreen::LoadStyle(): couldn't load default font.\n");
    exit(2);
  }
}


void BScreen::reconfigure(void) {
  LoadStyle();

  XGCValues gcv;
  gcv.foreground = WhitePixel(getBaseDisplay()->getXDisplay(),
			      getScreenNumber());
  gcv.function = GXinvert;
  gcv.subwindow_mode = IncludeInferiors;
  XChangeGC(getBaseDisplay()->getXDisplay(), opGC,
	    GCForeground | GCFunction | GCSubwindowMode, &gcv);
  
  gcv.foreground = resource.wstyle.l_text_focus.getPixel();
  gcv.font = resource.wstyle.font->fid;
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.wstyle.l_text_focus_gc,
	    GCForeground | GCFont, &gcv);
  
  gcv.foreground = resource.wstyle.l_text_unfocus.getPixel();
  gcv.font = resource.wstyle.font->fid;
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.wstyle.l_text_unfocus_gc,
	    GCForeground | GCFont, &gcv);

  gcv.foreground = resource.wstyle.b_pic_focus.getPixel();
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.wstyle.b_pic_focus_gc,
	    GCForeground, &gcv);

  gcv.foreground = resource.wstyle.b_pic_unfocus.getPixel();
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.wstyle.b_pic_unfocus_gc,
	    GCForeground, &gcv);

  gcv.foreground = resource.mstyle.t_text.getPixel();
  gcv.font = resource.mstyle.t_font->fid;
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.mstyle.t_text_gc,
	    GCForeground | GCFont, &gcv);
  
  gcv.foreground = resource.mstyle.f_text.getPixel();
  gcv.font = resource.mstyle.f_font->fid;
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.mstyle.f_text_gc,
	    GCForeground | GCFont, &gcv);
    
  gcv.foreground = resource.mstyle.h_text.getPixel();
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.mstyle.h_text_gc,
	    GCForeground | GCFont, &gcv);

  gcv.foreground = resource.mstyle.d_text.getPixel();
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.mstyle.d_text_gc,
	    GCForeground | GCFont, &gcv);

  gcv.foreground = resource.tstyle.l_text.getPixel();
  gcv.font = resource.tstyle.font->fid;
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.tstyle.l_text_gc,
	    GCForeground | GCFont, &gcv);

  gcv.foreground = resource.tstyle.w_text.getPixel();
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.tstyle.w_text_gc,
	    GCForeground | GCFont, &gcv);
  
  gcv.foreground = resource.tstyle.c_text.getPixel();
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.tstyle.c_text_gc,
	    GCForeground | GCFont, &gcv);

  gcv.foreground = resource.tstyle.b_pic.getPixel();
  XChangeGC(getBaseDisplay()->getXDisplay(), resource.tstyle.b_pic_gc,
	    GCForeground | GCFont, &gcv);
  
  geom_h = resource.wstyle.font->ascent +
    resource.wstyle.font->descent +
    (resource.bevel_width * 2);
  geom_w = (resource.bevel_width * 2) +
    XTextWidth(resource.wstyle.font, "0: 0000 x 0: 0000",
	       strlen("0: 0000 x 0: 0000"));
  
  Pixmap tmp = geom_pixmap;
  geom_pixmap =
    image_control->renderImage(geom_w, geom_h, &resource.wstyle.l_focus);
  if (tmp) image_control->removeImage(tmp);

  XSetWindowBackgroundPixmap(getBaseDisplay()->getXDisplay(), geom_window,
                             geom_pixmap);
  XSetWindowBorderWidth(getBaseDisplay()->getXDisplay(), geom_window,
                        resource.border_width);
  XSetWindowBorder(getBaseDisplay()->getXDisplay(), geom_window,
                   resource.border_color.getPixel());

  workspacemenu->reconfigure();
  iconmenu->reconfigure();
  
  InitMenu();
  rootmenu->reconfigure();
  configmenu->reconfigure();

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
  while (workspaceNames->count()) {
    char *n = workspaceNames->remove(0);

#ifdef    DEBUG
    deallocate(sizeof(char) * (strlen(n) + 1), "Screen.cc");
#endif // DEBUG
    
    delete [] n;
  }
}


void BScreen::LoadStyle(void) {
  resource.stylerc = XrmGetFileDatabase(blackbox->getStyleFilename());
  if (! resource.stylerc)
    resource.stylerc = XrmGetFileDatabase(DEFAULTSTYLE);

  XrmValue value;
  char *value_type;
  
  // load window config
  readDatabaseTexture("window.title.focus", "Window.Title.Focus",
		      &resource.wstyle.t_focus,
		      WhitePixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.title.unfocus", "Window.Title.Unfocus",
		      &resource.wstyle.t_unfocus,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.label.focus", "Window.Label.Focus",
		      &resource.wstyle.l_focus,
		      WhitePixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.label.unfocus", "Window.Label.Unfocus",
		      &resource.wstyle.l_unfocus,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.handle.focus", "Window.Handle.Focus",
		      &resource.wstyle.h_focus,
		      WhitePixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.handle.unfocus", "Window.Handle.Unfocus",
		      &resource.wstyle.h_unfocus,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.grip.focus", "Window.Grip.Focus",
                      &resource.wstyle.g_focus,
		      WhitePixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.grip.unfocus", "Window.Grip.Unfocus",
                      &resource.wstyle.g_unfocus,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.button.focus", "Window.Button.Focus",
		      &resource.wstyle.b_focus,
		      WhitePixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.button.unfocus", "Window.Button.Unfocus",
		      &resource.wstyle.b_unfocus,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.button.pressed", "Window.Button.Pressed",
		      &resource.wstyle.b_pressed,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.frame.focus", "Window.Frame.Focus",
		      &resource.wstyle.f_focus,
		      WhitePixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("window.frame.unfocus", "Window.Frame.Unfocus",
		      &resource.wstyle.f_unfocus,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseColor("window.label.focus.textColor",
		    "Window.Label.Focus.TextColor",
		    &resource.wstyle.l_text_focus,
		    BlackPixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseColor("window.label.unfocus.textColor",
		    "Window.Label.Unfocus.TextColor",
		    &resource.wstyle.l_text_unfocus,
		    WhitePixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseColor("window.button.focus.picColor",
		    "Window.Button.Focus.PicColor",
		    &resource.wstyle.b_pic_focus,
		    BlackPixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseColor("window.button.unfocus.picColor",
		    "Window.Button.Unfocus.PicColor",
		    &resource.wstyle.b_pic_unfocus,
		    WhitePixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseFont("window.font", "Window.Font",
		   &resource.wstyle.font);

  if (XrmGetResource(resource.stylerc, "window.justify", "Window.Justify",
		     &value_type, &value)) {
    if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
      resource.wstyle.justify = BScreen::RightJustify;
    else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
      resource.wstyle.justify = BScreen::CenterJustify;
    else
      resource.wstyle.justify = BScreen::LeftJustify;
  } else
    resource.wstyle.justify = BScreen::LeftJustify;
  
  // load toolbar config
  readDatabaseTexture("toolbar", "Toolbar",
		      &resource.tstyle.toolbar,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("toolbar.label", "Toolbar.Label",
		      &resource.tstyle.label,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("toolbar.windowLabel", "Toolbar.WindowLabel",
		      &resource.tstyle.window,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("toolbar.button", "Toolbar.Button",
		      &resource.tstyle.button,
		      WhitePixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("toolbar.button.pressed", "Toolbar.Button.Pressed",
		      &resource.tstyle.pressed,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("toolbar.clock", "Toolbar.Clock",
		      &resource.tstyle.clock,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseColor("toolbar.label.textColor", "Toolbar.Label.TextColor",
		    &resource.tstyle.l_text,
		    WhitePixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseColor("toolbar.windowLabel.textColor",
		    "Toolbar.WindowLabel.TextColor",
		    &resource.tstyle.w_text,
		    WhitePixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseColor("toolbar.clock.textColor", "Toolbar.Clock.TextColor",
		    &resource.tstyle.c_text,
		    WhitePixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseColor("toolbar.button.picColor", "Toolbar.Button.PicColor",
		    &resource.tstyle.b_pic,
		    BlackPixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseFont("toolbar.font", "Toolbar.Font", &resource.tstyle.font);
  
  if (XrmGetResource(resource.stylerc, "toolbar.justify",
		     "Toolbar.Justify", &value_type, &value)) {
    if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
      resource.tstyle.justify = BScreen::RightJustify;
    else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
      resource.tstyle.justify = BScreen::CenterJustify;
    else
      resource.tstyle.justify = BScreen::LeftJustify;
  } else
    resource.tstyle.justify = BScreen::LeftJustify;
  
  // load menu config
  readDatabaseTexture("menu.title", "Menu.Title",
		      &resource.mstyle.title,
		      WhitePixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("menu.frame", "Menu.Frame",
		      &resource.mstyle.frame,
		      BlackPixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseTexture("menu.hilite", "Menu.Hilite",
		      &resource.mstyle.hilite,
		      WhitePixel(getBaseDisplay()->getXDisplay(),
				 getScreenNumber()));
  readDatabaseColor("menu.title.textColor", "Menu.Title.TextColor",
		    &resource.mstyle.t_text,
		    BlackPixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseColor("menu.frame.textColor", "Menu.Frame.TextColor",
		    &resource.mstyle.f_text,
		    WhitePixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseColor("menu.frame.disableColor", "Menu.Frame.DisableColor",
		    &resource.mstyle.d_text,
		    BlackPixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseColor("menu.hilite.textColor", "Menu.Hilite.TextColor",
		    &resource.mstyle.h_text,
		    BlackPixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));
  readDatabaseFont("menu.title.font", "Menu.Title.Font",
		   &resource.mstyle.t_font);
  readDatabaseFont("menu.frame.font", "Menu.Frame.Font",
		   &resource.mstyle.f_font);

  if (XrmGetResource(resource.stylerc, "menu.title.justify",
		     "Menu.Title.Justify",
		     &value_type, &value)) {
    if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
      resource.mstyle.t_justify = BScreen::RightJustify;
    else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
      resource.mstyle.t_justify = BScreen::CenterJustify;
    else
      resource.mstyle.t_justify = BScreen::LeftJustify;
  } else
    resource.mstyle.t_justify = BScreen::LeftJustify;

  if (XrmGetResource(resource.stylerc, "menu.frame.justify",
		     "Menu.Frame.Justify",
		     &value_type, &value)) {
    if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
      resource.mstyle.f_justify = BScreen::RightJustify;
    else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
      resource.mstyle.f_justify = BScreen::CenterJustify;
    else
      resource.mstyle.f_justify = BScreen::LeftJustify;
  } else
    resource.mstyle.f_justify = BScreen::LeftJustify;

  if (XrmGetResource(resource.stylerc, "menu.bullet", "Menu.Bullet",
                     &value_type, &value)) {
    if (! strncasecmp(value.addr, "empty", value.size))
      resource.mstyle.bullet = Basemenu::Empty;
    else if (! strncasecmp(value.addr, "square", value.size))
      resource.mstyle.bullet = Basemenu::Square;
    else if (! strncasecmp(value.addr, "diamond", value.size))
      resource.mstyle.bullet = Basemenu::Diamond;
    else
      resource.mstyle.bullet = Basemenu::Triangle;
  } else
    resource.mstyle.bullet = Basemenu::Triangle;

  if (XrmGetResource(resource.stylerc, "menu.bullet.position",
                     "Menu.Bullet.Position", &value_type, &value)) {
    if (! strncasecmp(value.addr, "right", value.size))
      resource.mstyle.bullet_pos = Basemenu::Right;
    else
      resource.mstyle.bullet_pos = Basemenu::Left;
  } else
    resource.mstyle.bullet_pos = Basemenu::Left;

  readDatabaseColor("borderColor", "BorderColor", &resource.border_color,
		    BlackPixel(getBaseDisplay()->getXDisplay(),
			       getScreenNumber()));

  // load bevel, border and handle widths
  if (XrmGetResource(resource.stylerc, "handleWidth", "HandleWidth",
                     &value_type, &value)) {
    if (sscanf(value.addr, "%u", &resource.handle_width) != 1)
      resource.handle_width = 6;
    else
      if (resource.handle_width > (getWidth() / 2) ||
          resource.handle_width == 0)
	resource.handle_width = 6;
  } else
    resource.handle_width = 6;

  if (XrmGetResource(resource.stylerc, "borderWidth", "BorderWidth",
                     &value_type, &value)) {
    if (sscanf(value.addr, "%u", &resource.border_width) != 1)
      resource.border_width = 1;
  } else
    resource.border_width = 1;

  resource.border_width_2x = resource.border_width * 2;

  if (XrmGetResource(resource.stylerc, "bevelWidth", "BevelWidth",
                     &value_type, &value)) {
    if (sscanf(value.addr, "%u", &resource.bevel_width) != 1)
      resource.bevel_width = 3;
    else
      if (resource.bevel_width > (getWidth() / 2) || resource.bevel_width == 0)
	resource.bevel_width = 3;
  } else
    resource.bevel_width = 3;

  if (XrmGetResource(resource.stylerc,
                     "rootCommand",
                     "RootCommand", &value_type, &value)) {
#ifndef   __EMX__
    int dslen = strlen(DisplayString(getBaseDisplay()->getXDisplay()));
    
#  ifdef    DEBUG
    allocate(sizeof(char) * (dslen + 32), "Screen.cc");
#  endif // DEBUG

    char *displaystring = new char[dslen + 32];

#  ifdef    DEBUG
    allocate(sizeof(char) * (strlen(value.addr) + dslen + 64), "Screen.cc");
#  endif // DEBUG
    
    char *command = new char[strlen(value.addr) + dslen + 64];
    
    sprintf(displaystring, "%s",
	    DisplayString(getBaseDisplay()->getXDisplay()));
    // gotta love pointer math
    sprintf(displaystring + dslen - 1, "%d", getScreenNumber());
    sprintf(command, "DISPLAY=\"%s\" exec %s &",  displaystring, value.addr);
    system(command);
    
#  ifdef    DEBUG
    deallocate(sizeof(char) * (dslen + 32), "Screen.cc");
    deallocate(sizeof(char) * (strlen(value.addr) + dslen + 64), "Screen.cc");
#  endif // DEBUG
    
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

  toolbar->reconfigure();
  updateNetizenWorkspaceCount();
  
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

    toolbar->reconfigure();
    updateNetizenWorkspaceCount();
    
    return workspacesList->count();
  }
  
  return 0;
}


void BScreen::changeWorkspaceID(int id) {
  if (! current_workspace) return;

  if (id != current_workspace->getWorkspaceID()) {
    current_workspace->hideAll();

    workspacemenu->setItemSelected(current_workspace->getWorkspaceID() + 2,
				   False);
    
    if (blackbox->getFocusedWindow() &&
	blackbox->getFocusedWindow()->getScreen() == this &&
        (! blackbox->getFocusedWindow()->isStuck()))
      blackbox->setFocusedWindow((BlackboxWindow *) 0);
    current_workspace = getWorkspace(id);
    
    workspacemenu->setItemSelected(current_workspace->getWorkspaceID() + 2,
				   True);
    toolbar->redrawWorkspaceLabel(True);
    
    current_workspace->showAll();
  }

  updateNetizenCurrentWorkspace();
}


void BScreen::addNetizen(Netizen *n) {
  netizenList->insert(n);

  n->sendWorkspaceCount();
  n->sendCurrentWorkspace();

  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it++) {
    int i;
    for (i = 0; i < it.current()->getCount(); i++)
      n->sendWindowAdd(it.current()->getWindow(i)->getClientWindow(),
                       it.current()->getWorkspaceID());
  }

  Window f = ((blackbox->getFocusedWindow()) ?
              blackbox->getFocusedWindow()->getClientWindow() : None);
  n->sendWindowFocus(f);
}


void BScreen::removeNetizen(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  int i = 0;

  for (; it.current(); it++, i++)
    if (it.current()->getWindowID() == w) {
      Netizen *n = netizenList->remove(i);
      delete n;

      break;
    }
}


void BScreen::updateNetizenCurrentWorkspace(void) {
  LinkedListIterator<Netizen> it(netizenList);
  for (; it.current(); it++)
    it.current()->sendCurrentWorkspace();
}


void BScreen::updateNetizenWorkspaceCount(void) {
  LinkedListIterator<Netizen> it(netizenList);
  for (; it.current(); it++)
    it.current()->sendWorkspaceCount();
}


void BScreen::updateNetizenWindowFocus(void) {
  LinkedListIterator<Netizen> it(netizenList);
  Window f = ((blackbox->getFocusedWindow()) ?
              blackbox->getFocusedWindow()->getClientWindow() : None);
  for (; it.current(); it++)
    it.current()->sendWindowFocus(f);
}


void BScreen::updateNetizenWindowAdd(Window w, unsigned long p) {
  LinkedListIterator<Netizen> it(netizenList);
  for (; it.current(); it++)
    it.current()->sendWindowAdd(w, p);
}


void BScreen::updateNetizenWindowDel(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  for (; it.current(); it++)
    it.current()->sendWindowDel(w);
}


void BScreen::updateNetizenWindowRaise(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  for (; it.current(); it++)
    it.current()->sendWindowRaise(w);
}


void BScreen::updateNetizenWindowLower(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  for (; it.current(); it++)
    it.current()->sendWindowLower(w);
}


void BScreen::updateNetizenConfigNotify(XEvent *e) {
  LinkedListIterator<Netizen> it(netizenList);
  for (; it.current(); it++)
    it.current()->sendConfigNotify(e);
}


void BScreen::raiseWindows(Window *workspace_stack, int num) {
#ifdef    DEBUG
  allocate(sizeof(Window) * (num + workspacesList->count() + rootmenuList->count() + 13),
	   "Screen.cc");
#endif // DEBUG

  Window *session_stack = new
    Window[(num + workspacesList->count() + rootmenuList->count() + 13)];
  int i = 0, k = num;
  
  XRaiseWindow(getBaseDisplay()->getXDisplay(), iconmenu->getWindowID());
  *(session_stack + i++) = iconmenu->getWindowID();
  
  LinkedListIterator<Workspace> wit(workspacesList);
  for (; wit.current(); wit++)
    *(session_stack + i++) = wit.current()->getMenu()->getWindowID();
  
  *(session_stack + i++) = workspacemenu->getWindowID();
  
  *(session_stack + i++) = configmenu->getFocusmenu()->getWindowID();
  *(session_stack + i++) = configmenu->getPlacementmenu()->getWindowID();
  *(session_stack + i++) = configmenu->getWindowID();

#ifdef    SLIT
  *(session_stack + i++) = slit->getMenu()->getDirectionmenu()->getWindowID();
  *(session_stack + i++) = slit->getMenu()->getPlacementmenu()->getWindowID();
  *(session_stack + i++) = slit->getMenu()->getWindowID();
#endif // SLIT

  *(session_stack + i++) =
    toolbar->getMenu()->getPlacementmenu()->getWindowID();
  *(session_stack + i++) = toolbar->getMenu()->getWindowID();

  LinkedListIterator<Rootmenu> rit(rootmenuList);
  for (; rit.current(); rit++)
    *(session_stack + i++) = rit.current()->getWindowID();
  *(session_stack + i++) = rootmenu->getWindowID();

  if (toolbar->isOnTop())
    *(session_stack + i++) = toolbar->getWindowID();

#ifdef    SLIT
  if (slit->isOnTop())
    *(session_stack + i++) = slit->getWindowID();
#endif // SLIT
  
  while (k--)
    *(session_stack + i++) = *(workspace_stack + k);
  
  XRestackWindows(getBaseDisplay()->getXDisplay(), session_stack, i);

#ifdef    DEBUG
  deallocate(sizeof(Window) * (num + workspacesList->count() + rootmenuList->count() + 13),
	     "Screen.cc");
#endif // DEBUG

  delete [] session_stack;
}


#ifdef    HAVE_STRFTIME
void BScreen::saveStrftimeFormat(char *format) {
  if (resource.strftime_format) {
#  ifdef    DEBUG
    deallocate(sizeof(char) * (strlen(resource.strftime_format) + 1), "Screen.cc");
#  endif // DEBUG

    delete [] resource.strftime_format;
  }
  
#  ifdef    DEBUG
  allocate(sizeof(char) * (strlen(format) + 1), "Screen.cc");
#  endif // DEBUG

  resource.strftime_format = new char[strlen(format) + 1];
  sprintf(resource.strftime_format, "%s", format);
}
#endif // HAVE_STRFTIME


void BScreen::addWorkspaceName(char *name) {
  int nlen = strlen(name) + 1;

#ifdef    DEBUG
  allocate(sizeof(char) * nlen, "Screen.cc");
#endif // DEBUG
  
  char *wkspc_name = new char[nlen];
  strncpy(wkspc_name, name, nlen);
  
  workspaceNames->insert(wkspc_name);
}


void BScreen::getNameOfWorkspace(int id, char **name) {
  if (id >= 0 && id < workspaceNames->count()) {
    char *wkspc_name = workspaceNames->find(id);
    
    if (wkspc_name) {
      int len = strlen(wkspc_name) + 1;
      
      // we don't call allocate() here when debugging because the debugger keeps records only
      // on individual source files... this memory is allocated for Workspace.cc... which is
      // immediately freed... no leak here...
      //
      //      allocate(sizeof(char) * len, "Screen.cc");
      //

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
    rootmenu = new Rootmenu(this);
  
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
		  key += tolower(line[i]);
	    
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
    rootmenu->setInternalMenu();
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
	      key += tolower(line[i]);
	
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

	case 630: // config
	  {
	    if (! *label) {
	      fprintf(stderr, "BScreen::parseMenufile: [config] error, "
		      "no label defined");
	      continue;
	    }
	    
	    menu->insert(label, configmenu);
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

              Rootmenu *submenu = new Rootmenu(this);
	      
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

        case 995: // stylesdir 
        case 1113: // stylesmenu
          {
            Bool newmenu = ((key == 1113) ? True : False);

            if ((! *label) || ((! *command) && newmenu)) {
              fprintf(stderr, "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
                      " error, no directory defined\n");
              continue;
            }

            char stylesdir[MAXPATHLEN];

            char *directory = ((newmenu) ? command : label);
            int directory_length = ((newmenu) ? command_length : label_length);

            // perform shell style ~ home directory expansion
            char *homedir = 0;
            int homedir_len = 0;

            if (*directory == '~' && *(directory + 1) == '/') {
              homedir = getenv("HOME");
              homedir_len = strlen(homedir);
            }
             
            if (homedir && homedir_len != 0) {
              strncpy(stylesdir, homedir, homedir_len);

              strncpy(stylesdir + homedir_len, directory + 1,
                      directory_length - 1);
              *(stylesdir + directory_length + homedir_len - 1) = '\0';
            } else {
              strncpy(stylesdir, directory, directory_length);
              *(stylesdir + directory_length) = '\0';
            }

            struct stat statbuf;

            if (! stat(stylesdir, &statbuf)) {
              if (S_ISDIR(statbuf.st_mode)) {
                Rootmenu *stylesmenu;

                if (newmenu)
                  stylesmenu = new Rootmenu(this);
                else
                  stylesmenu = menu;

                DIR *d = opendir(stylesdir);
                int entries = 0;
                struct dirent *p;

                // get the total number of directory entries
                while ((p = readdir(d))) entries++;
                rewinddir(d);

#ifdef    DEBUG
		allocate(sizeof(char*) * entries, "Screen.cc");
#endif // DEBUG

                char **ls = new char* [entries];
                int index = 0;
                while ((p = readdir(d))) {
                  int len = strlen(p->d_name) + 1;

#ifdef    DEBUG
		  allocate(sizeof(char) * len, "Screen.cc");
#endif // DEBUG
		  
                  ls[index] = new char[len];
                  strncpy(ls[index++], p->d_name, len);
                }

                qsort(ls, entries, sizeof(char *), dcmp);

                int n, slen = strlen(stylesdir);
                for (n = 0; n < entries; n++) {
                  int nlen = strlen(ls[n]);
                  char style[MAXPATHLEN + 1];

                  strncpy(style, stylesdir, slen);
                  *(style + slen) = '/';
                  strncpy(style + slen + 1, ls[n], nlen + 1);

                  if ((! stat(style, &statbuf)) && S_ISREG(statbuf.st_mode))
                    stylesmenu->insert(ls[n], BScreen::SetStyle, style);

#ifdef    DEBUG
		  deallocate(sizeof(char) * (nlen + 1), "Screen.cc");
#endif // DEBUG
		  
                  delete [] ls[n];
                }
		
#ifdef    DEBUG
		deallocate(sizeof(char*) * entries, "Screen.cc");
#endif // DEBUG
		
                delete [] ls;
		
                stylesmenu->update();

                if (newmenu) {
                  stylesmenu->setLabel(label);
                  menu->insert(label, stylesmenu);
                  rootmenuList->insert(stylesmenu);
                }

                blackbox->saveMenuFilename(stylesdir);
              } else {
                fprintf(stderr, "BScreen::parseMenuFile:"
                        " [stylesdir/stylesmenu] error, %s is not a"
                        " directory\n", stylesdir);
              }
            } else {
              fprintf(stderr, "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
                      " error, %s does not exist\n", stylesdir);
            }

            break;
          }

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

  XSelectInput(getBaseDisplay()->getXDisplay(), getRootWindow(), NoEventMask);
  XSync(getBaseDisplay()->getXDisplay(), False);

  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it ++)
    it.current()->shutdown();
  
  blackbox->ungrab();
}


void BScreen::showPosition(int x, int y) {
  if (! geom_visible) {
    XMoveResizeWindow(getBaseDisplay()->getXDisplay(), geom_window,
                      (getWidth() - geom_w) / 2,
                      (getHeight() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(getBaseDisplay()->getXDisplay(), geom_window);
    XRaiseWindow(getBaseDisplay()->getXDisplay(), geom_window);

    geom_visible = True;
  }

  char label[1024];

  sprintf(label, "X: %4d x Y: %4d", x, y);

  XClearWindow(getBaseDisplay()->getXDisplay(), geom_window);
  XDrawString(getBaseDisplay()->getXDisplay(), geom_window,
	      resource.wstyle.l_text_focus_gc,
              resource.bevel_width,
	      resource.wstyle.font->ascent +
              resource.bevel_width, label, strlen(label));
}


void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
  if (! geom_visible) {
    XMoveResizeWindow(getBaseDisplay()->getXDisplay(), geom_window,
                      (getWidth() - geom_w) / 2,
                      (getHeight() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(getBaseDisplay()->getXDisplay(), geom_window);
    XRaiseWindow(getBaseDisplay()->getXDisplay(), geom_window);

    geom_visible = True;
  }

  char label[1024];

  sprintf(label, "W: %4d x H: %4d", gx, gy);

  XClearWindow(getBaseDisplay()->getXDisplay(), geom_window);
  XDrawString(getBaseDisplay()->getXDisplay(), geom_window,
	      resource.wstyle.l_text_focus_gc,
              resource.bevel_width,
	      resource.wstyle.font->ascent +
              resource.bevel_width, label, strlen(label));
}


void BScreen::hideGeometry(void) {
  if (geom_visible) {
    XUnmapWindow(getBaseDisplay()->getXDisplay(), geom_window);
    geom_visible = False;
  }
}


// Screen.cc for Blackbox - an X11 Window manager
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

#include "i18n.hh"
#include "blackbox.hh"
#include "Clientmenu.hh"
#include "GCCache.hh"
#include "Iconmenu.hh"
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
#  include <sys/types.h>
#endif // STDC_HEADERS

#ifdef    HAVE_CTYPE_H
#  include <ctype.h>
#endif // HAVE_CTYPE_H

#ifdef    HAVE_DIRENT_H
#  include <dirent.h>
#endif // HAVE_DIRENT_H

#ifdef    HAVE_LOCALE_H
#  include <locale.h>
#endif // HAVE_LOCALE_H

#ifdef    HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#ifdef    HAVE_STDARG_H
#  include <stdarg.h>
#endif // HAVE_STDARG_H

#ifndef    HAVE_SNPRINTF
#  include "bsd-snprintf.h"
#endif // !HAVE_SNPRINTF

#ifndef   MAXPATHLEN
#define   MAXPATHLEN 255
#endif // MAXPATHLEN

#ifndef   FONT_ELEMENT_SIZE
#define   FONT_ELEMENT_SIZE 50
#endif // FONT_ELEMENT_SIZE

#include <algorithm>
#include <iostream>

static Bool running = True;

static int anotherWMRunning(Display *display, XErrorEvent *) {
  fprintf(stderr, i18n->getMessage(ScreenSet, ScreenAnotherWMRunning,
     "BScreen::BScreen: an error occured while querying the X server.\n"
	     "  another window manager already running on display %s.\n"),
          DisplayString(display));

  running = False;

  return(-1);
}

struct dcmp {
  bool operator()(const char *one, const char *two) const {
    return (strcmp(one, two) < 0) ? True : False;
  }
};

#ifndef    HAVE_STRCASESTR
static const char * strcasestr(const char *str, const char *ptn) {
  const char *s2, *p2;
  for( ; *str; str++) {
    for(s2=str,p2=ptn; ; s2++,p2++) {
      if (!*p2) return str;
      if (toupper(*s2) != toupper(*p2)) break;
    }
  }
  return NULL;
}
#endif // HAVE_STRCASESTR

static const char *getFontElement(const char *pattern, char *buf, int bufsiz, ...) {
  const char *p, *v;
  char *p2;
  va_list va;

  va_start(va, bufsiz);
  buf[bufsiz-1] = 0;
  buf[bufsiz-2] = '*';
  while((v = va_arg(va, char *)) != NULL) {
    p = strcasestr(pattern, v);
    if (p) {
      strncpy(buf, p+1, bufsiz-2);
      p2 = strchr(buf, '-');
      if (p2) *p2=0;
      va_end(va);
      return p;
    }
  }
  va_end(va);
  strncpy(buf, "*", bufsiz);
  return NULL;
}

static const char *getFontSize(const char *pattern, int *size) {
  const char *p;
  const char *p2=NULL;
  int n=0;

  for (p=pattern; 1; p++) {
    if (!*p) {
      if (p2!=NULL && n>1 && n<72) {
	*size = n; return p2+1;
      } else {
	*size = 16; return NULL;
      }
    } else if (*p=='-') {
      if (n>1 && n<72 && p2!=NULL) {
	*size = n;
	return p2+1;
      }
      p2=p; n=0;
    } else if (*p>='0' && *p<='9' && p2!=NULL) {
      n *= 10;
      n += *p-'0';
    } else {
      p2=NULL; n=0;
    }
  }
}


BScreen::BScreen(Blackbox *bb, int scrn)
    : ScreenInfo(bb, scrn)
{
  blackbox = bb;

  event_mask = ColormapChangeMask | PropertyChangeMask |

	       EnterWindowMask | LeaveWindowMask |
	       SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
	       ButtonPressMask | ButtonReleaseMask;

  XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(*blackbox, rootWindow(), event_mask);
  XSync(*blackbox, False);
  XSetErrorHandler((XErrorHandler) old);

  managed = running;
  if (! managed) return;

  rootmenu = 0;
  resource.stylerc = 0;

  resource.mstyle.t_fontset = resource.mstyle.f_fontset =
    resource.tstyle.fontset = resource.wstyle.fontset = (XFontSet) 0;
  resource.mstyle.t_font = resource.mstyle.f_font = resource.tstyle.font =
    resource.wstyle.font = (XFontStruct *) 0;

#ifdef    HAVE_STRFTIME
  resource.strftime_format = 0;
#endif // HAVE_STRFTIME

#ifdef    HAVE_GETPID
  pid_t bpid = getpid();

  XChangeProperty(*blackbox, rootWindow(),
                  blackbox->getBlackboxPidAtom(), XA_CARDINAL,
                  sizeof(pid_t) * 8, PropModeReplace,
                  (unsigned char *) &bpid, 1);
#endif // HAVE_GETPID

  XDefineCursor(*blackbox, rootWindow(),
                blackbox->getSessionCursor());

  workspaceNames = new LinkedList<char>;
  workspacesList = new LinkedList<Workspace>;
  rootmenuList = new LinkedList<Rootmenu>;
  netizenList = new LinkedList<Netizen>;
  iconList = new LinkedList<BlackboxWindow>;
  strutList = new LinkedList<NETStrut>;

  // start off full screen, top left.
  usableArea.x = usableArea.y = 0;
  usableArea.width = width();
  usableArea.height = height();

  image_control =
    new BImageControl(blackbox, this, True, blackbox->getColorsPerChannel(),
                      blackbox->getCacheLife(), blackbox->getCacheMax());
  image_control->installRootColormap();
  root_colormap_installed = True;

  blackbox->load_rc(this);

  image_control->setDither(resource.image_dither);

  LoadStyle();

  const char *s =  i18n->getMessage(ScreenSet, ScreenPositionLength,
				    "0: 0000 x 0: 0000");
  int l = strlen(s);

  if (i18n->multibyte()) {
    XRectangle ink, logical;
    XmbTextExtents(resource.wstyle.fontset, s, l, &ink, &logical);
    geom_w = logical.width;

    geom_h = resource.wstyle.fontset_extents->max_ink_extent.height;
  } else {
    geom_h = resource.wstyle.font->ascent +
	     resource.wstyle.font->descent;

    geom_w = XTextWidth(resource.wstyle.font, s, l);
  }

  geom_w += (resource.bevel_width * 2);
  geom_h += (resource.bevel_width * 2);

  XSetWindowAttributes attrib;
  unsigned long mask = CWBorderPixel | CWColormap | CWSaveUnder;
  attrib.border_pixel = getBorderColor()->pixel();
  attrib.colormap = colormap();
  attrib.save_under = True;

  geom_window =
    XCreateWindow(*blackbox, rootWindow(),
                  0, 0, geom_w, geom_h, resource.border_width, depth(),
                  InputOutput, visual(), mask, &attrib);
  geom_visible = False;

  if (resource.wstyle.l_focus.texture() & BImage_ParentRelative) {
    if (resource.wstyle.t_focus.texture() ==
	                              (BImage_Flat | BImage_Solid)) {
      geom_pixmap = None;
      XSetWindowBackground(*blackbox, geom_window,
			   resource.wstyle.t_focus.color().pixel());
    } else {
      geom_pixmap = image_control->renderImage(geom_w, geom_h,
					       resource.wstyle.t_focus);
      XSetWindowBackgroundPixmap(*blackbox,
				 geom_window, geom_pixmap);
    }
  } else {
    if (resource.wstyle.l_focus.texture() ==
	                              (BImage_Flat | BImage_Solid)) {
      geom_pixmap = None;
      XSetWindowBackground(*blackbox, geom_window,
			   resource.wstyle.l_focus.color().pixel());
    } else {
      geom_pixmap = image_control->renderImage(geom_w, geom_h,
					       resource.wstyle.l_focus);
      XSetWindowBackgroundPixmap(*blackbox,
				 geom_window, geom_pixmap);
    }
  }

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

  workspacemenu->insert(i18n->getMessage(IconSet, IconIcons, "Icons"),
			iconmenu);
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

  updateAvailableArea();

  changeWorkspaceID(0);

  int i;
  unsigned int nchild;
  Window r, p, *children;
  XQueryTree(*blackbox, rootWindow(), &r, &p,
	     &children, &nchild);

  // preen the window list of all icon windows... for better dockapp support
  for (i = 0; i < (int) nchild; i++) {
    if (children[i] == None) continue;

    XWMHints *wmhints = XGetWMHints(*blackbox,
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
    if (XGetWindowAttributes(*blackbox, children[i],
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

  XFree(children);

  // call this again just in case a window we found updates the Strut list
  updateAvailableArea();

  if (! resource.sloppy_focus)
    XSetInputFocus(*blackbox, toolbar->getWindowID(),
		   RevertToParent, CurrentTime);

  XFlush(*blackbox);
}


BScreen::~BScreen(void) {
  if (! managed) return;

  if (geom_pixmap != None)
    image_control->removeImage(geom_pixmap);

  if (geom_window != None)
    XDestroyWindow(*blackbox, geom_window);

  removeWorkspaceNames();

  while (workspacesList->count())
    delete workspacesList->remove(0);

  while (rootmenuList->count())
    rootmenuList->remove(0);

  while (iconList->count())
    delete iconList->remove(0);

  while (netizenList->count())
    delete netizenList->remove(0);

#ifdef    HAVE_STRFTIME
  if (resource.strftime_format)
    delete [] resource.strftime_format;
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
  delete iconList;
  delete netizenList;
  delete strutList;

  if (resource.wstyle.fontset)
    XFreeFontSet(*blackbox, resource.wstyle.fontset);
  if (resource.mstyle.t_fontset)
    XFreeFontSet(*blackbox, resource.mstyle.t_fontset);
  if (resource.mstyle.f_fontset)
    XFreeFontSet(*blackbox, resource.mstyle.f_fontset);
  if (resource.tstyle.fontset)
    XFreeFontSet(*blackbox, resource.tstyle.fontset);

  if (resource.wstyle.font)
    XFreeFont(*blackbox, resource.wstyle.font);
  if (resource.mstyle.t_font)
    XFreeFont(*blackbox, resource.mstyle.t_font);
  if (resource.mstyle.f_font)
    XFreeFont(*blackbox, resource.mstyle.f_font);
  if (resource.tstyle.font)
      XFreeFont(*blackbox, resource.tstyle.font);
}

BTexture BScreen::readDatabaseTexture( const char *rname, const char *rclass,
				       const char *default_color )
{
    BTexture texture;
    XrmValue value;
    char *value_type;

    if (XrmGetResource(resource.stylerc, rname, rclass, &value_type,
		       &value))
	texture = BTexture( value.addr );
    else
	texture.setTexture(BImage_Solid | BImage_Flat);

    if (texture.texture() & BImage_Solid) {
	int clen = strlen(rclass) + 32, nlen = strlen(rname) + 32;

	char *colorclass = new char[clen], *colorname = new char[nlen];

	sprintf(colorclass, "%s.Color", rclass);
	sprintf(colorname,  "%s.color", rname);

	texture.setColor( readDatabaseColor(colorname, colorclass, default_color) );

#ifdef    INTERLACE
	sprintf(colorclass, "%s.ColorTo", rclass);
	sprintf(colorname,  "%s.colorTo", rname);
	texture.setColorTo( readDatabaseColor(colorname, colorclass, default_color) );
#endif // INTERLACE

	delete [] colorclass;
	delete [] colorname;

	unsigned char r, g, b;
	r = texture.color().red() | (texture.color().red() >> 1);
	g = texture.color().green() | (texture.color().green() >> 1);
	b = texture.color().blue() | (texture.color().blue() >> 1);
	texture.setLightColor( BColor( r, g, b ) );

	r = (texture.color().red() >> 2) | (texture.color().red() >> 1);
	g = (texture.color().green() >> 2) | (texture.color().green() >> 1);
	b = (texture.color().blue() >> 2) | (texture.color().blue() >> 1);
	texture.setShadowColor( BColor( r, g, b ) );
    } else if (texture.texture() & BImage_Gradient) {
	int clen = strlen(rclass) + 10, nlen = strlen(rname) + 10;

	char *colorclass = new char[clen], *colorname = new char[nlen],
	   *colortoclass = new char[clen], *colortoname = new char[nlen];

	sprintf(colorclass, "%s.Color", rclass);
	sprintf(colorname,  "%s.color", rname);

	sprintf(colortoclass, "%s.ColorTo", rclass);
	sprintf(colortoname,  "%s.colorTo", rname);

	texture.setColor( readDatabaseColor(colorname, colorclass,
					    default_color) );
	texture.setColorTo( readDatabaseColor(colortoname, colortoclass,
					      default_color) );

	delete [] colorclass;
	delete [] colorname;
	delete [] colortoclass;
	delete [] colortoname;
    }

    texture.setScreen( screen() );
    return texture;
}


BColor BScreen::readDatabaseColor( const char *rname, const char *rclass,
				   const char *default_color)
{
    BColor color( default_color );

    XrmValue value;
    char *value_type;
    if (XrmGetResource(resource.stylerc, rname, rclass, &value_type, &value))
	color = BColor( value.addr );

    color.setScreen( screen() );
    return color;
}


void BScreen::readDatabaseFontSet( const char *rname, const char *rclass,
				  XFontSet *fontset )
{
    if (! fontset)
	return;

    static char *defaultFont = "fixed";

    Bool load_default = False;
    XrmValue value;
    char *value_type;

    if (*fontset)
	XFreeFontSet(*blackbox, *fontset);

    if (XrmGetResource(resource.stylerc, rname, rclass, &value_type, &value)) {
	char *fontname = value.addr;
	if (! (*fontset = createFontSet(fontname)))
	    load_default = True;
    } else {
	load_default = True;
    }

    if (load_default) {
	*fontset = createFontSet(defaultFont);

	if (! *fontset) {
	    fprintf(stderr, i18n->getMessage(ScreenSet, ScreenDefaultFontLoadFail,
					     "BScreen::LoadStyle(): couldn't load default font.\n"));
	    exit(2);
	}
    }
}


void BScreen::readDatabaseFont( const char *rname, const char *rclass,
				XFontStruct **font)
{
    if (! font)
	return;

    static char *defaultFont = "fixed";

    Bool load_default = False;
    XrmValue value;
    char *value_type;

    if (*font)
	XFreeFont(*blackbox, *font);

    if (XrmGetResource(resource.stylerc, rname, rclass, &value_type, &value)) {
	if ((*font = XLoadQueryFont(*blackbox,
				    value.addr)) == NULL) {
	    fprintf(stderr, i18n->getMessage(ScreenSet, ScreenFontLoadFail,
					     "BScreen::LoadStyle(): couldn't load font '%s'\n"),
		    value.addr);

	    load_default = True;
	}
    } else {
	load_default = True;
    }

    if (load_default) {
	if ((*font = XLoadQueryFont(*blackbox,
				    defaultFont)) == NULL) {
	    fprintf(stderr, i18n->getMessage(ScreenSet, ScreenDefaultFontLoadFail,
					     "BScreen::LoadStyle(): couldn't load default font.\n"));
	    exit(2);
	}
    }
}


XFontSet BScreen::createFontSet( const char *fontname )
{
    XFontSet fs;
    char **missing, *def = "-";
    int nmissing, pixel_size = 0, buf_size = 0;
    char weight[FONT_ELEMENT_SIZE], slant[FONT_ELEMENT_SIZE];

    fs = XCreateFontSet(*blackbox,
			fontname, &missing, &nmissing, &def);
    if (fs && (! nmissing)) return fs;

#ifdef    HAVE_SETLOCALE
    if (! fs) {
	if (nmissing) XFreeStringList(missing);

	setlocale(LC_CTYPE, "C");
	fs = XCreateFontSet(*blackbox, fontname,
			    &missing, &nmissing, &def);
	setlocale(LC_CTYPE, "");
    }
#endif // HAVE_SETLOCALE

    if (fs) {
	XFontStruct **fontstructs;
	char **fontnames;
	XFontsOfFontSet(fs, &fontstructs, &fontnames);
	fontname = fontnames[0];
    }

    getFontElement(fontname, weight, FONT_ELEMENT_SIZE,
		   "-medium-", "-bold-", "-demibold-", "-regular-", NULL);
    getFontElement(fontname, slant, FONT_ELEMENT_SIZE,
		   "-r-", "-i-", "-o-", "-ri-", "-ro-", NULL);
    getFontSize(fontname, &pixel_size);

    if (! strcmp(weight, "*")) strncpy(weight, "medium", FONT_ELEMENT_SIZE);
    if (! strcmp(slant, "*")) strncpy(slant, "r", FONT_ELEMENT_SIZE);
    if (pixel_size < 3) pixel_size = 3;
    else if (pixel_size > 97) pixel_size = 97;

    buf_size = strlen(fontname) + (FONT_ELEMENT_SIZE * 2) + 64;
    char *pattern2 = new char[buf_size];
    snprintf(pattern2, buf_size - 1,
	     "%s,"
	     "-*-*-%s-%s-*-*-%d-*-*-*-*-*-*-*,"
	     "-*-*-*-*-*-*-%d-*-*-*-*-*-*-*,*",
	     fontname, weight, slant, pixel_size, pixel_size);
    fontname = pattern2;

    if (nmissing) XFreeStringList(missing);
    if (fs) XFreeFontSet(*blackbox, fs);

    fs = XCreateFontSet(*blackbox, fontname,
			&missing, &nmissing, &def);
    delete [] pattern2;

    return fs;
}


void BScreen::reconfigure(void) {
  LoadStyle();

  const char *s = i18n->getMessage(ScreenSet, ScreenPositionLength,
				   "0: 0000 x 0: 0000");
  int l = strlen(s);

  if (i18n->multibyte()) {
    XRectangle ink, logical;
    XmbTextExtents(resource.wstyle.fontset, s, l, &ink, &logical);
    geom_w = logical.width;

    geom_h = resource.wstyle.fontset_extents->max_ink_extent.height;
  } else {
    geom_w = XTextWidth(resource.wstyle.font, s, l);

    geom_h = resource.wstyle.font->ascent +
	     resource.wstyle.font->descent;
  }

  geom_w += (resource.bevel_width * 2);
  geom_h += (resource.bevel_width * 2);

  Pixmap tmp = geom_pixmap;
  if (resource.wstyle.l_focus.texture() & BImage_ParentRelative) {
    if (resource.wstyle.t_focus.texture() == (BImage_Flat | BImage_Solid)) {
      geom_pixmap = None;
      XSetWindowBackground(*blackbox, geom_window,
			 resource.wstyle.t_focus.color().pixel());
    } else {
      geom_pixmap = image_control->renderImage(geom_w, geom_h,
					       resource.wstyle.t_focus);
      XSetWindowBackgroundPixmap(*blackbox,
				 geom_window, geom_pixmap);
    }
  } else {
    if (resource.wstyle.l_focus.texture() == (BImage_Flat | BImage_Solid)) {
      geom_pixmap = None;
      XSetWindowBackground(*blackbox, geom_window,
			 resource.wstyle.l_focus.color().pixel());
    } else {
      geom_pixmap = image_control->renderImage(geom_w, geom_h,
					       resource.wstyle.l_focus);
      XSetWindowBackgroundPixmap(*blackbox,
				 geom_window, geom_pixmap);
    }
  }
  if (tmp) image_control->removeImage(tmp);

  XSetWindowBorderWidth(*blackbox, geom_window,
                        resource.border_width);
  XSetWindowBorder(*blackbox, geom_window,
                   resource.border_color.pixel());

  workspacemenu->reconfigure();
  iconmenu->reconfigure();

  {
    int remember_sub = rootmenu->getCurrentSubmenu();
    InitMenu();
    raiseWindows(0, 0);
    rootmenu->reconfigure();
    rootmenu->drawSubmenu(remember_sub);
  }

  configmenu->reconfigure();

  toolbar->reconfigure();

#ifdef    SLIT
  slit->reconfigure();
#endif // SLIT

  LinkedListIterator<Workspace> wit(workspacesList);
  for (Workspace *w = wit.current(); w; wit++, w = wit.current())
    w->reconfigure();

  LinkedListIterator<BlackboxWindow> iit(iconList);
  for (BlackboxWindow *bw = iit.current(); bw; iit++, bw = iit.current())
    if (bw->validateClient())
      bw->reconfigure();

  image_control->timeout();
}


void BScreen::rereadMenu(void) {
  InitMenu();
  raiseWindows(0, 0);

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

    // load fonts/fontsets

    if (i18n->multibyte()) {
	readDatabaseFontSet("window.font", "Window.Font",
			    &resource.wstyle.fontset);
	readDatabaseFontSet("toolbar.font", "Toolbar.Font",
			    &resource.tstyle.fontset);
	readDatabaseFontSet("menu.title.font", "Menu.Title.Font",
			    &resource.mstyle.t_fontset);
	readDatabaseFontSet("menu.frame.font", "Menu.Frame.Font",
			    &resource.mstyle.f_fontset);

	resource.mstyle.t_fontset_extents =
	    XExtentsOfFontSet(resource.mstyle.t_fontset);
	resource.mstyle.f_fontset_extents =
	    XExtentsOfFontSet(resource.mstyle.f_fontset);
	resource.tstyle.fontset_extents =
	    XExtentsOfFontSet(resource.tstyle.fontset);
	resource.wstyle.fontset_extents =
	    XExtentsOfFontSet(resource.wstyle.fontset);
    } else {
	readDatabaseFont("window.font", "Window.Font",
			 &resource.wstyle.font);
	readDatabaseFont("menu.title.font", "Menu.Title.Font",
			 &resource.mstyle.t_font);
	readDatabaseFont("menu.frame.font", "Menu.Frame.Font",
			 &resource.mstyle.f_font);
	readDatabaseFont("toolbar.font", "Toolbar.Font",
			 &resource.tstyle.font);
    }

    // load window config
    resource.wstyle.t_focus =
	readDatabaseTexture("window.title.focus", "Window.Title.Focus", "white" );

    resource.wstyle.t_unfocus =
	readDatabaseTexture("window.title.unfocus", "Window.Title.Unfocus", "black" );
    resource.wstyle.l_focus =
	readDatabaseTexture("window.label.focus", "Window.Label.Focus", "white" );
    resource.wstyle.l_unfocus =
	readDatabaseTexture("window.label.unfocus", "Window.Label.Unfocus", "black" );
    resource.wstyle.h_focus =
	readDatabaseTexture("window.handle.focus", "Window.Handle.Focus", "white" );
    resource.wstyle.h_unfocus =
	readDatabaseTexture("window.handle.unfocus", "Window.Handle.Unfocus", "black" );
    resource.wstyle.g_focus =
	readDatabaseTexture("window.grip.focus", "Window.Grip.Focus", "white" );
    resource.wstyle.g_unfocus =
	readDatabaseTexture("window.grip.unfocus", "Window.Grip.Unfocus", "black" );
    resource.wstyle.b_focus =
	readDatabaseTexture("window.button.focus", "Window.Button.Focus", "white" );
    resource.wstyle.b_unfocus =
	readDatabaseTexture("window.button.unfocus", "Window.Button.Unfocus", "black" );
    resource.wstyle.b_pressed =
	readDatabaseTexture("window.button.pressed", "Window.Button.Pressed", "white" );
    resource.wstyle.f_focus =
	readDatabaseColor("window.frame.focusColor",
			  "Window.Frame.FocusColor", "white" );
    resource.wstyle.f_unfocus =
	readDatabaseColor("window.frame.unfocusColor",
			  "Window.Frame.UnfocusColor", "black" );
    resource.wstyle.l_text_focus =
	readDatabaseColor("window.label.focus.textColor",
			  "Window.Label.Focus.TextColor", "white" );
    resource.wstyle.l_text_unfocus =
	readDatabaseColor("window.label.unfocus.textColor",
			  "Window.Label.Unfocus.TextColor", "white" );
    resource.wstyle.b_pic_focus =
	readDatabaseColor("window.button.focus.picColor",
			  "Window.Button.Focus.PicColor", "black" );
    resource.wstyle.b_pic_unfocus =
	readDatabaseColor("window.button.unfocus.picColor",
			  "Window.Button.Unfocus.PicColor", "white" );

    if (XrmGetResource(resource.stylerc, "window.justify", "Window.Justify",
		       &value_type, &value)) {
	if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
	    resource.wstyle.justify = BScreen::RightJustify;
	else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
	    resource.wstyle.justify = BScreen::CenterJustify;
	else
	    resource.wstyle.justify = BScreen::LeftJustify;
    } else {
	resource.wstyle.justify = BScreen::LeftJustify;
    }
    // load toolbar config
    resource.tstyle.toolbar =
	readDatabaseTexture("toolbar", "Toolbar", "black" );
    resource.tstyle.label =
	readDatabaseTexture("toolbar.label", "Toolbar.Label", "black" );
    resource.tstyle.window =
	readDatabaseTexture("toolbar.windowLabel", "Toolbar.WindowLabel", "black" );
    resource.tstyle.button =
	readDatabaseTexture("toolbar.button", "Toolbar.Button", "white" );
    resource.tstyle.pressed =
	readDatabaseTexture("toolbar.button.pressed",
			    "Toolbar.Button.Pressed", "white" );
    resource.tstyle.clock =
	readDatabaseTexture("toolbar.clock", "Toolbar.Clock", "black" );
    resource.tstyle.l_text =
	readDatabaseColor("toolbar.label.textColor",
			  "Toolbar.Label.TextColor", "white" );
    resource.tstyle.w_text =
	readDatabaseColor("toolbar.windowLabel.textColor",
			  "Toolbar.WindowLabel.TextColor", "white" );
    resource.tstyle.c_text =
	readDatabaseColor("toolbar.clock.textColor",
			  "Toolbar.Clock.TextColor", "white" );
    resource.tstyle.b_pic =
	readDatabaseColor("toolbar.button.picColor",
			  "Toolbar.Button.PicColor", "black" );

    if (XrmGetResource(resource.stylerc, "toolbar.justify",
		       "Toolbar.Justify", &value_type, &value)) {
	if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
	    resource.tstyle.justify = BScreen::RightJustify;
	else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
	    resource.tstyle.justify = BScreen::CenterJustify;
	else
	    resource.tstyle.justify = BScreen::LeftJustify;
    } else {
	resource.tstyle.justify = BScreen::LeftJustify;
    }
    // load menu config
    resource.mstyle.title =
	readDatabaseTexture("menu.title", "Menu.Title", "white" );
    resource.mstyle.frame =
	readDatabaseTexture("menu.frame", "Menu.Frame", "black" );
    resource.mstyle.hilite =
	readDatabaseTexture("menu.hilite", "Menu.Hilite", "white" );
    resource.mstyle.t_text =
	readDatabaseColor("menu.title.textColor", "Menu.Title.TextColor", "black" );
    resource.mstyle.f_text =
	readDatabaseColor("menu.frame.textColor", "Menu.Frame.TextColor", "white" );
    resource.mstyle.d_text =
	readDatabaseColor("menu.frame.disableColor",
			  "Menu.Frame.DisableColor", "black" );
    resource.mstyle.h_text =
	readDatabaseColor("menu.hilite.textColor", "Menu.Hilite.TextColor", "black" );

    if (XrmGetResource(resource.stylerc, "menu.title.justify",
		       "Menu.Title.Justify",
		       &value_type, &value)) {
	if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
	    resource.mstyle.t_justify = BScreen::RightJustify;
	else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
	    resource.mstyle.t_justify = BScreen::CenterJustify;
	else
	    resource.mstyle.t_justify = BScreen::LeftJustify;
    } else {
	resource.mstyle.t_justify = BScreen::LeftJustify;
    }
    if (XrmGetResource(resource.stylerc, "menu.frame.justify",
		       "Menu.Frame.Justify",
		       &value_type, &value)) {
	if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
	    resource.mstyle.f_justify = BScreen::RightJustify;
	else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
	    resource.mstyle.f_justify = BScreen::CenterJustify;
	else
	    resource.mstyle.f_justify = BScreen::LeftJustify;
    } else {
	resource.mstyle.f_justify = BScreen::LeftJustify;
    }
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
    } else {
	resource.mstyle.bullet = Basemenu::Triangle;
    }
    if (XrmGetResource(resource.stylerc, "menu.bullet.position",
		       "Menu.Bullet.Position", &value_type, &value)) {
	if (! strncasecmp(value.addr, "right", value.size))
	    resource.mstyle.bullet_pos = Basemenu::Right;
	else
	    resource.mstyle.bullet_pos = Basemenu::Left;
    } else {
	resource.mstyle.bullet_pos = Basemenu::Left;
    }
    resource.border_color =
	readDatabaseColor("borderColor", "BorderColor", "black" );

    // load bevel, border and handle widths
    if (XrmGetResource(resource.stylerc, "handleWidth", "HandleWidth",
		       &value_type, &value)) {
	if (sscanf(value.addr, "%u", &resource.handle_width) != 1 ||
	    resource.handle_width > width() / 2 || resource.handle_width == 0)
	    resource.handle_width = 6;
    } else {
	resource.handle_width = 6;
    }
    if (XrmGetResource(resource.stylerc, "borderWidth", "BorderWidth",
		       &value_type, &value)) {
	if (sscanf(value.addr, "%u", &resource.border_width) != 1)
	    resource.border_width = 1;
    } else {
	resource.border_width = 1;
    }

    if (XrmGetResource(resource.stylerc, "bevelWidth", "BevelWidth",
		       &value_type, &value)) {
	if (sscanf(value.addr, "%u", &resource.bevel_width) != 1 ||
	    resource.bevel_width > width() / 2 || resource.bevel_width == 0)
	    resource.bevel_width = 3;
    } else {
	resource.bevel_width = 3;
    }
    if (XrmGetResource(resource.stylerc, "frameWidth", "FrameWidth",
		       &value_type, &value)) {
	if (sscanf(value.addr, "%u", &resource.frame_width) != 1 ||
	    resource.frame_width > width() / 2)
	    resource.frame_width = resource.bevel_width;
    } else {
	resource.frame_width = resource.bevel_width;
    }
    if (XrmGetResource(resource.stylerc,
		       "rootCommand",
		       "RootCommand", &value_type, &value)) {
#ifndef    __EMX__
	char displaystring[MAXPATHLEN];
	sprintf(displaystring, "DISPLAY=%s",
		DisplayString(blackbox->x11Display()));
	sprintf(displaystring + strlen(displaystring) - 1, "%d",
		screen());

	bexec(value.addr, displaystring);
#else //   __EMX__
	spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", value.addr, NULL);
#endif // !__EMX__
    }

    XrmDestroyDatabase(resource.stylerc);
}


void BScreen::addIcon(BlackboxWindow *w) {
  if (! w) return;

  w->setWorkspace(-1);
  w->setWindowNumber(iconList->count());

  iconList->insert(w);

  iconmenu->insert((const char **) w->getIconTitle());
  iconmenu->update();
}


void BScreen::removeIcon(BlackboxWindow *w) {
  if (! w) return;

  iconList->remove(w->getWindowNumber());

  iconmenu->remove(w->getWindowNumber());
  iconmenu->update();

  LinkedListIterator<BlackboxWindow> it(iconList);
  BlackboxWindow *bw = it.current();
  for (int i = 0; bw; it++, bw = it.current())
    bw->setWindowNumber(i++);
}


BlackboxWindow *BScreen::getIcon(int index) {
  if (index >= 0 && index < iconList->count())
    return iconList->find(index);

  return (BlackboxWindow *) 0;
}


int BScreen::addWorkspace(void) {
  Workspace *wkspc = new Workspace(this, workspacesList->count());
  workspacesList->insert(wkspc);

  workspacemenu->insert(wkspc->getName(), wkspc->getMenu(),
			wkspc->getWorkspaceID() + 2);
  workspacemenu->update();

  toolbar->reconfigure();

  updateNetizenWorkspaceCount();

  return workspacesList->count();
}


int BScreen::removeLastWorkspace(void) {
  if (workspacesList->count() == 1)
    return 0;

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


void BScreen::changeWorkspaceID(int id) {
  if (! current_workspace) return;

  if (id != current_workspace->getWorkspaceID()) {
    current_workspace->hideAll();

    workspacemenu->setItemSelected(current_workspace->getWorkspaceID() + 2,
				   False);

    if (blackbox->getFocusedWindow() &&
	blackbox->getFocusedWindow()->getScreen() == this &&
        (! blackbox->getFocusedWindow()->isStuck())) {
      current_workspace->setLastFocusedWindow(blackbox->getFocusedWindow());
      blackbox->setFocusedWindow((BlackboxWindow *) 0);
    }

    current_workspace = getWorkspace(id);

    workspacemenu->setItemSelected(current_workspace->getWorkspaceID() + 2,
				   True);
    toolbar->redrawWorkspaceLabel(True);

    current_workspace->showAll();

    if (resource.focus_last && current_workspace->getLastFocusedWindow()) {
      XSync(*blackbox, False);
      current_workspace->getLastFocusedWindow()->setInputFocus();
    }
  }

  updateNetizenCurrentWorkspace();
}


void BScreen::addNetizen(Netizen *n) {
  netizenList->insert(n);

  n->sendWorkspaceCount();
  n->sendCurrentWorkspace();

  LinkedListIterator<Workspace> it(workspacesList);
  for (Workspace *w = it.current(); w; it++, w = it.current()) {
    for (int i = 0; i < w->getCount(); i++)
      n->sendWindowAdd(w->getWindow(i)->getClientWindow(),
		       w->getWorkspaceID());
  }

  Window f = ((blackbox->getFocusedWindow()) ?
              blackbox->getFocusedWindow()->getClientWindow() : None);
  n->sendWindowFocus(f);
}


void BScreen::removeNetizen(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  int i = 0;

  for (Netizen *n = it.current(); n; it++, i++, n = it.current())
    if (n->windowID() == w) {
      Netizen *tmp = netizenList->remove(i);
      delete tmp;

      break;
    }
}


void BScreen::updateNetizenCurrentWorkspace(void) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendCurrentWorkspace();
}


void BScreen::updateNetizenWorkspaceCount(void) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWorkspaceCount();
}


void BScreen::updateNetizenWindowFocus(void) {
  Window f = ((blackbox->getFocusedWindow()) ?
              blackbox->getFocusedWindow()->getClientWindow() : None);
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWindowFocus(f);
}


void BScreen::updateNetizenWindowAdd(Window w, unsigned long p) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWindowAdd(w, p);
}


void BScreen::updateNetizenWindowDel(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWindowDel(w);
}


void BScreen::updateNetizenWindowRaise(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWindowRaise(w);
}


void BScreen::updateNetizenWindowLower(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWindowLower(w);
}


void BScreen::updateNetizenConfigNotify(XEvent *e) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendConfigNotify(e);
}


void BScreen::raiseWindows(Window *workspace_stack, int num) {
  Window *session_stack = new
    Window[(num + workspacesList->count() + rootmenuList->count() + 13)];
  int i = 0, k = num;

  XRaiseWindow(*blackbox, iconmenu->getWindowID());
  *(session_stack + i++) = iconmenu->getWindowID();

  LinkedListIterator<Workspace> wit(workspacesList);
  for (Workspace *tmp = wit.current(); tmp; wit++, tmp = wit.current())
    *(session_stack + i++) = tmp->getMenu()->getWindowID();

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
  for (Rootmenu *tmp = rit.current(); tmp; rit++, tmp = rit.current())
    *(session_stack + i++) = tmp->getWindowID();
  *(session_stack + i++) = rootmenu->getWindowID();

  if (toolbar->isOnTop())
    *(session_stack + i++) = toolbar->getWindowID();

#ifdef    SLIT
  if (slit->isOnTop())
    *(session_stack + i++) = slit->getWindowID();
#endif // SLIT

  while (k--)
    *(session_stack + i++) = *(workspace_stack + k);

  XRestackWindows(*blackbox, session_stack, i);

  delete [] session_stack;
}


#ifdef    HAVE_STRFTIME
void BScreen::saveStrftimeFormat(char *format) {
  if (resource.strftime_format)
    delete [] resource.strftime_format;

  resource.strftime_format = bstrdup(format);
}
#endif // HAVE_STRFTIME


void BScreen::addWorkspaceName(char *name) {
  workspaceNames->insert(bstrdup(name));
}


char* BScreen::getNameOfWorkspace(int id) {
  char *name = (char *) 0;

  if (id >= 0 && id < workspaceNames->count()) {
    char *wkspc_name = workspaceNames->find(id);

    if (wkspc_name)
      name = wkspc_name;
  }
  return name;
}


void BScreen::reassociateWindow(BlackboxWindow *w, int wkspc_id, Bool ignore_sticky) {
  if (! w) return;

  if (wkspc_id == -1)
    wkspc_id = current_workspace->getWorkspaceID();

  if (w->getWorkspaceNumber() == wkspc_id)
    return;

  if (w->isIconic()) {
    removeIcon(w);
    getWorkspace(wkspc_id)->addWindow(w);
  } else if (ignore_sticky || ! w->isStuck()) {
    getWorkspace(w->getWorkspaceNumber())->removeWindow(w);
    getWorkspace(wkspc_id)->addWindow(w);
  }
}


void BScreen::nextFocus(void) {
  Bool have_focused = False;
  int focused_window_number = -1;
  BlackboxWindow *next;

  if (blackbox->getFocusedWindow()) {
    if (blackbox->getFocusedWindow()->getScreen()->screen() ==
	screen()) {
      have_focused = True;
      focused_window_number = blackbox->getFocusedWindow()->getWindowNumber();
    }
  }

  if ((getCurrentWorkspace()->getCount() > 1) && have_focused) {
    int next_window_number = focused_window_number;
    do {
      if ((++next_window_number) >= getCurrentWorkspace()->getCount())
	next_window_number = 0;

      next = getCurrentWorkspace()->getWindow(next_window_number);
    } while ((! next->setInputFocus()) && (next_window_number !=
					   focused_window_number));

    if (next_window_number != focused_window_number)
      getCurrentWorkspace()->raiseWindow(next);
  } else if (getCurrentWorkspace()->getCount() >= 1) {
    next = current_workspace->getWindow(0);

    current_workspace->raiseWindow(next);
    next->setInputFocus();
  }
}


void BScreen::prevFocus(void) {
  Bool have_focused = False;
  int focused_window_number = -1;
  BlackboxWindow *prev;

  if (blackbox->getFocusedWindow()) {
    if (blackbox->getFocusedWindow()->getScreen()->screen() ==
	screen()) {
      have_focused = True;
      focused_window_number = blackbox->getFocusedWindow()->getWindowNumber();
    }
  }

  if ((getCurrentWorkspace()->getCount() > 1) && have_focused) {
    int prev_window_number = focused_window_number;
    do {
      if ((--prev_window_number) < 0)
	prev_window_number = getCurrentWorkspace()->getCount() - 1;

      prev = getCurrentWorkspace()->getWindow(prev_window_number);
    } while ((! prev->setInputFocus()) && (prev_window_number !=
					   focused_window_number));

    if (prev_window_number != focused_window_number)
      getCurrentWorkspace()->raiseWindow(prev);
  } else if (getCurrentWorkspace()->getCount() >= 1) {
    prev = current_workspace->getWindow(0);

    current_workspace->raiseWindow(prev);
    prev->setInputFocus();
  }
}


void BScreen::raiseFocus(void) {
  Bool have_focused = False;
  int focused_window_number = -1;

  if (blackbox->getFocusedWindow()) {
    if (blackbox->getFocusedWindow()->getScreen()->screen() ==
	screen()) {
      have_focused = True;
      focused_window_number = blackbox->getFocusedWindow()->getWindowNumber();
    }
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
  } else {
    rootmenu = new Rootmenu(this);
  }
  Bool defaultMenu = True;

  if (blackbox->getMenuFilename()) {
    FILE *menu_file = fopen(blackbox->getMenuFilename(), "r");

    if (!menu_file) {
      perror(blackbox->getMenuFilename());
    } else {
      if (feof(menu_file)) {
	fprintf(stderr, i18n->getMessage(ScreenSet, ScreenEmptyMenuFile,
					 "%s: Empty menu file"),
		blackbox->getMenuFilename());
      } else {
	char line[1024], label[1024];
	memset(line, 0, 1024);
	memset(label, 0, 1024);

	while (fgets(line, 1024, menu_file) && ! feof(menu_file)) {
	  if (line[0] != '#') {
	    int i, key = 0, index = -1, len = strlen(line);

	    key = 0;
	    for (i = 0; i < len; i++) {
	      if (line[i] == '[') index = 0;
	      else if (line[i] == ']') break;
	      else if (line[i] != ' ')
		if (index++ >= 0)
		  key += tolower(line[i]);
	    }

	    if (key == 517) {
	      index = -1;
	      for (i = index; i < len; i++) {
		if (line[i] == '(') index = 0;
		else if (line[i] == ')') break;
		else if (index++ >= 0) {
		  if (line[i] == '\\' && i < len - 1) i++;
		  label[index - 1] = line[i];
		}
	      }

	      if (index == -1) index = 0;
	      label[index] = '\0';

	      rootmenu->setLabel(label);
	      defaultMenu = parseMenuFile(menu_file, rootmenu);
	      break;
	    }
	  }
	}
      }
      fclose(menu_file);
    }
  }

  if (defaultMenu) {
    rootmenu->setInternalMenu();
    rootmenu->insert(i18n->getMessage(ScreenSet, Screenxterm, "xterm"),
		     BScreen::Execute,
		     i18n->getMessage(ScreenSet, Screenxterm, "xterm"));
    rootmenu->insert(i18n->getMessage(ScreenSet, ScreenRestart, "Restart"),
		     BScreen::Restart);
    rootmenu->insert(i18n->getMessage(ScreenSet, ScreenExit, "Exit"),
		     BScreen::Exit);
  } else {
    blackbox->saveMenuFilename(blackbox->getMenuFilename());
  }
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
	  line_length = strlen(line),
	  label_length = 0, command_length = 0;

	// determine the keyword
	key = 0;
	for (i = 0; i < line_length; i++) {
	  if (line[i] == '[') parse = 1;
	  else if (line[i] == ']') break;
	  else if (line[i] != ' ')
	    if (parse)
	      key += tolower(line[i]);
	}

	// get the label enclosed in ()'s
	parse = 0;

	for (i = 0; i < line_length; i++) {
	  if (line[i] == '(') {
	    index = 0;
	    parse = 1;
	  } else if (line[i] == ')') break;
	  else if (index++ >= 0) {
	    if (line[i] == '\\' && i < line_length - 1) i++;
	    label[index - 1] = line[i];
	  }
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
	for (i = 0; i < line_length; i++) {
	  if (line[i] == '{') {
	    index = 0;
	    parse = 1;
	  } else if (line[i] == '}') break;
	  else if (index++ >= 0) {
	    if (line[i] == '\\' && i < line_length - 1) i++;
	    command[index - 1] = line[i];
	  }
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
	    fprintf(stderr, i18n->getMessage(ScreenSet, ScreenEXECError,
			     "BScreen::parseMenuFile: [exec] error, "
			     "no menu label and/or command defined\n"));
	    continue;
	  }

	  menu->insert(label, BScreen::Execute, command);

	  break;

	case 442: // exit
	  if (! *label) {
	    fprintf(stderr, i18n->getMessage(ScreenSet, ScreenEXITError,
				     "BScreen::parseMenuFile: [exit] error, "
				     "no menu label defined\n"));
	    continue;
	  }

	  menu->insert(label, BScreen::Exit);

	  break;

	case 561: // style
	  {
	    if ((! *label) || (! *command)) {
	      fprintf(stderr, i18n->getMessage(ScreenSet, ScreenSTYLEError,
				 "BScreen::parseMenuFile: [style] error, "
				 "no menu label and/or filename defined\n"));
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
	  if (! *label) {
	    fprintf(stderr, i18n->getMessage(ScreenSet, ScreenCONFIGError,
			       "BScreen::parseMenufile: [config] error, "
			       "no label defined"));
	    continue;
	  }

	  menu->insert(label, configmenu);

	  break;

	case 740: // include
	  {
	    if (! *label) {
	      fprintf(stderr, i18n->getMessage(ScreenSet, ScreenINCLUDEError,
				 "BScreen::parseMenuFile: [include] error, "
				 "no filename defined\n"));
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
                struct stat buf;
                if (fstat(fileno(submenufile), &buf) ||
                    (! S_ISREG(buf.st_mode))) {
                  fprintf(stderr,
			  i18n->getMessage(ScreenSet, ScreenINCLUDEErrorReg,
			     "BScreen::parseMenuFile: [include] error: "
			     "'%s' is not a regular file\n"), newfile);
                  break;
                }

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
	      fprintf(stderr, i18n->getMessage(ScreenSet, ScreenSUBMENUError,
				 "BScreen::parseMenuFile: [submenu] error, "
				 "no menu label defined\n"));
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
	      fprintf(stderr, i18n->getMessage(ScreenSet, ScreenRESTARTError,
				 "BScreen::parseMenuFile: [restart] error, "
				 "no menu label defined\n"));
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
	      fprintf(stderr, i18n->getMessage(ScreenSet, ScreenRECONFIGError,
				 "BScreen::parseMenuFile: [reconfig] error, "
				 "no menu label defined\n"));
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
              fprintf(stderr,
		      i18n->getMessage(ScreenSet, ScreenSTYLESDIRError,
			 "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
			 " error, no directory defined\n"));
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

                char **ls = new char* [entries];
                int index = 0;
                while ((p = readdir(d)))
		  ls[index++] = bstrdup(p->d_name);

		closedir(d);

		std::sort(ls, ls + entries, dcmp());

                int n, slen = strlen(stylesdir);
                for (n = 0; n < entries; n++) {
                  if (ls[n][strlen(ls[n])-1] != '~') {
                    int nlen = strlen(ls[n]);
                    char style[MAXPATHLEN + 1];

                    strncpy(style, stylesdir, slen);
                    *(style + slen) = '/';
                    strncpy(style + slen + 1, ls[n], nlen + 1);

                    if ((! stat(style, &statbuf)) && S_ISREG(statbuf.st_mode))
                      stylesmenu->insert(ls[n], BScreen::SetStyle, style);
                  }

                  delete [] ls[n];
                }

                delete [] ls;

                stylesmenu->update();

                if (newmenu) {
                  stylesmenu->setLabel(label);
                  menu->insert(label, stylesmenu);
                  rootmenuList->insert(stylesmenu);
                }

                blackbox->saveMenuFilename(stylesdir);
              } else {
                fprintf(stderr, i18n->getMessage(ScreenSet,
						 ScreenSTYLESDIRErrorNotDir,
				   "BScreen::parseMenuFile:"
				   " [stylesdir/stylesmenu] error, %s is not a"
				   " directory\n"), stylesdir);
              }
            } else {
              fprintf(stderr,
		      i18n->getMessage(ScreenSet, ScreenSTYLESDIRErrorNoExist,
			 "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
			 " error, %s does not exist\n"), stylesdir);
            }

            break;
          }

	case 1090: // workspaces
	  {
	    if (! *label) {
	      fprintf(stderr,
		      i18n->getMessage(ScreenSet, ScreenWORKSPACESError,
			       "BScreen:parseMenuFile: [workspaces] error, "
			       "no menu label defined\n"));
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
  XGrabServer( *blackbox );

  XSelectInput(*blackbox, rootWindow(), NoEventMask);
  XSync(*blackbox, False);

  LinkedListIterator<Workspace> it(workspacesList);
  for (Workspace *w = it.current(); w; it++, w = it.current())
    w->shutdown();

  while (iconList->count()) {
    iconList->first()->restore();
    delete iconList->first();
  }

#ifdef    SLIT
  slit->shutdown();
#endif // SLIT

  XUngrabServer( *blackbox );
}


void BScreen::showPosition(int x, int y) {
  if (! geom_visible) {
    XMoveResizeWindow(*blackbox, geom_window,
                      (width() - geom_w) / 2,
                      (height() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(*blackbox, geom_window);
    XRaiseWindow(*blackbox, geom_window);

    geom_visible = True;
  }

  char label[1024];

  sprintf(label, i18n->getMessage(ScreenSet, ScreenPositionFormat,
				  "X: %4d x Y: %4d"), x, y);

  XClearWindow(*blackbox, geom_window);

  BGCCache::Item &gc = BGCCache::instance()->find( resource.wstyle.l_text_focus,
						   resource.wstyle.font );
  if (i18n->multibyte()) {
    XmbDrawString(*blackbox, geom_window,
		  resource.wstyle.fontset, gc.gc(),
		  resource.bevel_width, resource.bevel_width -
		  resource.wstyle.fontset_extents->max_ink_extent.y,
		  label, strlen(label));
  } else {
    XDrawString(*blackbox, geom_window, gc.gc(),
		resource.bevel_width,
		resource.wstyle.font->ascent + resource.bevel_width,
		label, strlen(label));
  }
  BGCCache::instance()->release( gc );
}


void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
  if (! geom_visible) {
    XMoveResizeWindow(*blackbox, geom_window,
                      (width() - geom_w) / 2,
                      (height() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(*blackbox, geom_window);
    XRaiseWindow(*blackbox, geom_window);

    geom_visible = True;
  }

  char label[1024];

  sprintf(label, i18n->getMessage(ScreenSet, ScreenGeometryFormat,
				  "W: %4d x H: %4d"), gx, gy);

  XClearWindow(*blackbox, geom_window);

  BGCCache::Item &gc = BGCCache::instance()->find( resource.wstyle.l_text_focus,
						   resource.wstyle.font );
  if (i18n->multibyte()) {
    XmbDrawString(*blackbox, geom_window,
		  resource.wstyle.fontset, gc.gc(),
		  resource.bevel_width, resource.bevel_width -
		  resource.wstyle.fontset_extents->max_ink_extent.y,
		  label, strlen(label));
  } else {
    XDrawString(*blackbox, geom_window, gc.gc(),
		resource.bevel_width, resource.wstyle.font->ascent +
		resource.bevel_width, label, strlen(label));
  }
}


void BScreen::hideGeometry(void) {
  if (geom_visible) {
    XUnmapWindow(*blackbox, geom_window);
    geom_visible = False;
  }
}

void BScreen::addStrut(NETStrut *strut) {
  strutList->insert(strut);
}

const XRectangle& BScreen::availableArea(void) const {
  if (doFullMax())
    return rect(); // return the full screen
  return usableArea;
}

void BScreen::updateAvailableArea(void)
{
    int old_x = usableArea.x, old_y = usableArea.y,
    old_width = usableArea.width, old_height = usableArea.height;

    LinkedListIterator<NETStrut> it(strutList);

    usableArea = rect(); // reset to full screen
    for(NETStrut *strut = it.current(); strut; ++it, strut = it.current()) {
	if (strut->left > usableArea.x)
	    usableArea.x = strut->left;

	if (strut->top > usableArea.y)
	    usableArea.y = strut->top;

	if (((usableArea.width + old_x) - strut->right) < usableArea.width)
	    usableArea.width = width() - strut->right - usableArea.x;

	if (((usableArea.height + old_y) - strut->bottom) < usableArea.height)
	    usableArea.height = height() - strut->bottom - usableArea.y;
    }

    // if area changed
    if (old_x != usableArea.x || old_y != usableArea.y ||
	old_width != usableArea.width || old_height != usableArea.height) {
	usableArea.width += old_x - usableArea.x;
	usableArea.height += old_y - usableArea.y;
	// TODO: update maximized windows to reflect new screen area
    }
}

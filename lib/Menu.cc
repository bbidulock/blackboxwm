// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Menu.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 Bradley T Hughes <bhughes at trolltech.com>
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

extern "C" {
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <assert.h>
}

#include "Menu.hh"
#include "Application.hh"
#include "Pen.hh"
#include "Resource.hh"

static const unsigned int arrow_width  = 7;
static const unsigned int arrow_height = 7;
static const char arrow_bits[] =
  { 0x00, 0x10, 0x30, 0x70, 0x30, 0x10, 0x00 };

static const unsigned int check_width  = 7;
static const unsigned int check_height = 7;
static const char check_bits[] =
  { 0x40, 0x60, 0x71, 0x3b, 0x1f, 0x0e, 0x04 };

static const unsigned int close_width  = 7;
static const unsigned int close_height = 7;
static const char close_bits[] =
  { 0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41 };


bt::MenuStyle **bt::MenuStyle::styles = 0;


bt::MenuStyle *bt::MenuStyle::get(Application &app,
                                  unsigned int screen,
                                  ImageControl *imagecontrol) {
  if (! styles) {
    styles = new MenuStyle*[app.getNumberOfScreens()];
    for (unsigned int i = 0; i < app.getNumberOfScreens(); ++i)
      styles[i] = 0;
  }
  if (! styles[screen])
    styles[screen] = new MenuStyle(app, screen, imagecontrol);
  return styles[screen];
}


bt::MenuStyle::MenuStyle(Application &app, unsigned int screen,
                         ImageControl *imagecontrol)
  : _app(app), _screen(screen), _imagecontrol(imagecontrol) {
  title.alignment = AlignLeft;
  frame.alignment = AlignLeft;
  margin_w = 1u;

  const ScreenInfo &screeninfo = _app.getDisplay().screenInfo(_screen);

  bitmap.arrow =
    XCreateBitmapFromData(_app.getXDisplay(), screeninfo.rootWindow(),
                          arrow_bits, arrow_width, arrow_height);
  bitmap.check =
    XCreateBitmapFromData(_app.getXDisplay(), screeninfo.rootWindow(),
                          check_bits, check_width, check_height);
  bitmap.close =
    XCreateBitmapFromData(_app.getXDisplay(), screeninfo.rootWindow(),
                          close_bits, close_width, close_height);
}


bt::MenuStyle::~MenuStyle(void) {
  if (bitmap.arrow) XFreePixmap(_app.getXDisplay(), bitmap.arrow);
  if (bitmap.check) XFreePixmap(_app.getXDisplay(), bitmap.check);
  if (bitmap.close) XFreePixmap(_app.getXDisplay(), bitmap.close);
}


void bt::MenuStyle::load(const Resource &resource) {
  // menu textures
  title.texture =
    textureResource(_app.getDisplay(), _screen, resource,
                    "menu.title", "Menu.Title", "black");
  frame.texture =
    textureResource(_app.getDisplay(), _screen, resource,
                    "menu.frame", "Menu.Frame", "white");
  active.texture =
    textureResource(_app.getDisplay(), _screen, resource,
                    "menu.active", "Menu.Active", "black");

  // non-texture colors
  title.foreground =
    Color::namedColor(_app.getDisplay(), _screen,
                      resource.read("menu.title.foregroundColor",
                                    "Menu.Title.ForegroundColor",
                                    "white"));
  title.text =
    Color::namedColor(_app.getDisplay(), _screen,
                      resource.read("menu.title.textColor",
                                    "Menu.Title.TextColor",
                                    "white"));
  frame.foreground =
    Color::namedColor(_app.getDisplay(), _screen,
                      resource.read("menu.frame.foregroundColor",
                                    "Menu.Frame.ForegroundColor",
                                    "black"));
  frame.text =
    Color::namedColor(_app.getDisplay(), _screen,
                      resource.read("menu.frame.textColor",
                                    "Menu.Frame.TextColor",
                                    "black"));
  frame.disabled =
    Color::namedColor(_app.getDisplay(), _screen,
                      resource.read("menu.frame.disabledColor",
                                    "Menu.Frame.DisabledColor",
                                    "black"));
  active.foreground =
    Color::namedColor(_app.getDisplay(), _screen,
                      resource.read("menu.active.foregroundColor",
                                    "Menu.Active.ForegroundColor",
                                    "white"));
  active.text =
    Color::namedColor(_app.getDisplay(), _screen,
                      resource.read("menu.active.textColor",
                                    "Menu.Active.TextColor",
                                    "white"));

  // fonts
  title.font.setFontName(resource.read("menu.title.font", "Menu.Title.Font"));
  frame.font.setFontName(resource.read("menu.frame.font", "Menu.Frame.Font"));

  item_indent = std::max(textHeight(frame.font), 7u);

  title.alignment =
    alignResource(resource, "menu.title.alignment",
                  "Menu.Title.Alignment");

  frame.alignment =
    alignResource(resource, "menu.frame.alignment",
                  "Menu.Frame.Alignment");

  const std::string mstr =
    resource.read("menu.marginWidth", "Menu.MarginWidth", "1");
  margin_w =
    std::max(static_cast<unsigned int>(strtoul(mstr.c_str(), 0, 0)), 1u);
}


unsigned int bt::MenuStyle::separatorHeight(void) const {
  return (frame.texture.borderWidth() > 0 ?
          frame.texture.borderWidth() : 1) + (margin_w * 2);
}


unsigned int bt::MenuStyle::titleMargin(void) const {
  return title.texture.borderWidth() + margin_w;
}


unsigned int bt::MenuStyle::frameMargin(void) const {
  return frame.texture.borderWidth() + margin_w;
}


unsigned int bt::MenuStyle::itemMargin(void) const {
  return active.texture.borderWidth() + margin_w;
}


Pixmap bt::MenuStyle::titlePixmap(unsigned int width, unsigned int height,
                                  Pixmap oldpixmap) {
  return title.texture.render(_app.getDisplay(), _screen, *_imagecontrol,
                              width, height, oldpixmap);
}


Pixmap bt::MenuStyle::framePixmap(unsigned int width, unsigned int height,
                                  Pixmap oldpixmap) {
  return frame.texture.render(_app.getDisplay(), _screen, *_imagecontrol,
                              width, height, oldpixmap);
}

Pixmap bt::MenuStyle::activePixmap(unsigned int width, unsigned int height,
                                   Pixmap oldpixmap) {
  return active.texture.render(_app.getDisplay(), _screen, *_imagecontrol,
                               width, height, oldpixmap);
}


bt::Rect bt::MenuStyle::titleRect(const std::string &text) const {
  const Rect &rect = textRect(title.font, text);
  return Rect(0, 0,
              rect.width()  + (titleMargin() * 2),
              rect.height() + (titleMargin() * 2));
}


bt::Rect bt::MenuStyle::itemRect(const MenuItem &item) const {
  const Rect &rect = textRect(frame.font, item.label());
  return Rect(0, 0,
              rect.width() + ((item_indent + itemMargin()) * 2),
              std::max(rect.height(), item_indent) + (itemMargin() * 2));
}


void bt::MenuStyle::drawTitle(Window window, const Rect &rect,
                              const std::string &text) const {
  Pen pen(_screen, title.text);
  Rect r;
  r.setCoords(rect.left() + titleMargin(), rect.top(),
              rect.right() - titleMargin(), rect.bottom());
  drawText(title.font, pen, window, r, title.alignment, text);
}


void bt::MenuStyle::drawItem(Window window, const Rect &rect,
                             const MenuItem &item, Pixmap pixmap) const {
  Rect r2;
  r2.setCoords(rect.left() + item_indent, rect.top(),
               rect.right() - item_indent, rect.bottom());

  if (item.isSeparator()) {
    Pen pen(_screen, frame.foreground);
    XFillRectangle(_app.getXDisplay(), window, pen.gc(),
                   r2.x(), r2.y() + margin_w, r2.width(),
                   (frame.texture.borderWidth() > 0 ?
                    frame.texture.borderWidth() : 1));
    return;
  }

  Pen fpen(_screen, (item.isEnabled() ? (item.isActive() ? active.foreground :
                                         frame.foreground) : frame.disabled));
  Pen tpen(_screen, (item.isEnabled() ? (item.isActive() ? active.text :
                                         frame.text) : frame.disabled));
  if (item.isActive() && item.isEnabled()) {
    Pen p2(_screen, active.texture.color());
    if (pixmap) {
      XCopyArea(_app.getXDisplay(), pixmap, window, p2.gc(),
                0, 0, rect.width(), rect.height(), rect.x(), rect.y());
    } else {
      XFillRectangle(_app.getXDisplay(), window, p2.gc(),
                     rect.x(), rect.y(), rect.width(), rect.height());
    }
  }
  drawText(frame.font, tpen, window, r2, frame.alignment, item.label());

  if (item.isChecked()) {
    // draw check mark
    int cx = rect.x() + (rect.height() - 7) / 2;
    int cy = rect.y() + (rect.height() - 7) / 2;

    XSetClipMask(_app.getXDisplay(), fpen.gc(), bitmap.check);
    XSetClipOrigin(_app.getXDisplay(), fpen.gc(), cx, cy);
    XFillRectangle(_app.getXDisplay(), window, fpen.gc(), cx, cy, 7, 7);
  }

  if (item.submenu()) {
    // draw submenu arrow
    int ax = rect.x() + rect.width() - rect.height() + (rect.height() - 7) / 2;
    int ay = rect.y() + (rect.height() - 7) / 2;

    XSetClipMask(_app.getXDisplay(), fpen.gc(), bitmap.arrow);
    XSetClipOrigin(_app.getXDisplay(), fpen.gc(), ax, ay);
    XFillRectangle(_app.getXDisplay(), window, fpen.gc(), ax, ay, 7, 7);
  }

  XSetClipOrigin(_app.getXDisplay(), fpen.gc(), 0, 0);
  XSetClipMask(_app.getXDisplay(), fpen.gc(), None);
}


namespace bt {
  // internal helper classes

  class ShowDelay : public TimeoutHandler {
  public:
    Menu *showmenu;
    inline ShowDelay(void) : showmenu(0) { }
    inline void timeout(void) {
      if (showmenu) showmenu->show();
      showmenu = 0;
    }
  };

  class IdentMatch {
  public:
    unsigned int _id;
    inline IdentMatch(unsigned int id) : _id(id) { }
    inline bool operator()(const MenuItem &item) const
    { return item.id() == _id; }
  };

  class IndexMatch {
  public:
    unsigned int _index;
    inline IndexMatch(unsigned int index) : _index(index) { }
    inline bool operator()(const MenuItem &item) const
    { return item.index() == _index; }
  };

  class InteractMatch {
  public:
    inline bool operator()(const MenuItem &item) const
    { return item.isEnabled() && ! item.isSeparator(); }
  };

} // namespace bt

static bt::ShowDelay showdelay;


bt::Menu::Menu(Application &app, unsigned int screen)
  : _app(app),
    _screen(screen),
    _tpixmap(0),
    _fpixmap(0),
    _apixmap(0),
    _rect(0, 0, 1, 1),
    _trect(0, 0, 0, 0),
    _frect(0, 0, 1, 1),
    _irect(0, 0, 1, 1),
    _timer(&_app, &showdelay),
    _parent_menu(0),
    _active_submenu(0),
    _motion(0),
    _itemw(1),
    _active_index(~0u),
    _auto_delete(false),
    _pressed(false),
    _title_pressed(false),
    _size_dirty(true),
    _show_title(false),
    _visible(false)
{
  idset.reset();

  const ScreenInfo& screeninfo = _app.getDisplay().screenInfo(_screen);

  // create the window
  XSetWindowAttributes attrib;
  unsigned long mask = CWBackPixmap | CWColormap |
                       CWOverrideRedirect | CWEventMask;
  attrib.background_pixmap = None;
  attrib.colormap = screeninfo.colormap();
  attrib.override_redirect = False;
  attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
                      ButtonMotionMask | PointerMotionMask |
                      KeyPressMask | LeaveWindowMask | ExposureMask;
  attrib.override_redirect = True;

  _window =
    XCreateWindow(_app.getXDisplay(), screeninfo.rootWindow(),
                  _rect.x(), _rect.y(), _rect.width(), _rect.height(), 0,
                  screeninfo.depth(), InputOutput,
                  screeninfo.visual(), mask, &attrib);
  _app.insertEventHandler(_window, this);

  _timer.setTimeout(200);
}


bt::Menu::~Menu(void)
{
  hide();
  clear();

  _app.removeEventHandler(_window);
  XDestroyWindow(_app.getXDisplay(), _window);
}


unsigned int bt::Menu::insertItem(const MenuItem &item,
                                  unsigned int id, unsigned int index) {
  // hardcoded maximum of 500 items
  assert(items.size() < 500);

  ItemList::iterator it;
  if (index == ~0u) {
    // append the new item
    index = items.size();
    it = items.end();
  } else {
    index = std::min(index, items.size());
    it = items.begin();
    std::advance<ItemList::iterator, signed>(it, index);
  }

  it = items.insert(it, item);
  if (! item.separator) it->ident = verifyId(id);
  it->indx = index;

  if (isVisible()) {
    updateSize();
    XClearArea(_app.getXDisplay(), _window, 0, 0,
               _rect.width(), _rect.height(), True);
  } else {
    _size_dirty = true;
  }

  return it->ident;
}


unsigned int bt::Menu::insertItem(const std::string &label,
                                  unsigned int id, unsigned int index) {
  return insertItem(MenuItem(MenuItem::Normal, label), id, index);
}


unsigned int bt::Menu::insertItem(const std::string &label, Menu *submenu,
                                  unsigned int id, unsigned int index) {
  submenu->_parent_menu = this;
  return insertItem(MenuItem(submenu, label), id, index);
}


void bt::Menu::insertSeparator(unsigned int index) {
  (void) insertItem(MenuItem(MenuItem::Separator), ~0u, index);
}


void bt::Menu::changeItem(unsigned int id, const std::string &newlabel,
                          unsigned int newid) {
  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  ItemList::iterator it, end;
  for (it = items.begin(), end = items.end(); it != end; ++it) {
    r.setHeight(it->height);

    if (! it->separator && it->ident == id) {
      // new label
      it->lbl = newlabel;
      if (newid != ~0u) {
        // change the id if necessary
        idset.reset(it->ident);
        it->ident = verifyId(newid);
      }
      if (isVisible())
        XClearArea(_app.getXDisplay(), _window,
                   r.x(), r.y(), r.width(), r.height(), True);
      break;
    }

    r.setY(r.y() + r.height());
    ++row;

    if (r.y() >= _irect.bottom()) {
      // next column
      ++col;
      row = 0;
      r.setPos(_irect.x() + (_itemw * col), _irect.y());
    }
  }
}


void bt::Menu::setItemEnabled(unsigned int id, bool enabled) {
  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  ItemList::iterator it, end;
  for (it = items.begin(), end = items.end(); it != end; ++it) {
    r.setHeight(it->height);

    if (it->id() == id) {
      // found the item, change the status and redraw if visible
      it->enabled = enabled;
      if (isVisible())
        XClearArea(_app.getXDisplay(), _window,
                   r.x(), r.y(), r.width(), r.height(), True);
      break;
    }

    r.setY(r.y() + r.height());
    ++row;

    if (r.y() >= _irect.bottom()) {
      // next column
      ++col;
      row = 0;
      r.setPos(_irect.x() + (_itemw * col), _irect.y());
    }
  }
}


bool bt::Menu::isItemEnabled(unsigned int id) const {
  ItemList::const_iterator it =
    std::find_if(items.begin(), items.end(), IdentMatch(id));
  return (it != items.end() && it->enabled);
}


void bt::Menu::setItemChecked(unsigned int id, bool checked) {
  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  ItemList::iterator it, end;
  for (it = items.begin(), end = items.end(); it != end; ++it) {
    r.setHeight(it->height);

    if (it->id() == id) {
      // found the item, change the status and redraw if visible
      it->checked = checked;
      if (isVisible())
        XClearArea(_app.getXDisplay(), _window,
                   r.x(), r.y(), r.width(), r.height(), True);
      break;
    }

    r.setY(r.y() + r.height());
    ++row;

    if (r.y() >= _irect.bottom()) {
      // next column
      ++col;
      row = 0;
      r.setPos(_irect.x() + (_itemw * col), _irect.y());
    }
  }
}


bool bt::Menu::isItemChecked(unsigned int id) const {
  ItemList::const_iterator it =
    std::find_if(items.begin(), items.end(), IdentMatch(id));
  return (it != items.end() && it->checked);
}


void bt::Menu::removeItem(unsigned int id) {
  ItemList::iterator it =
    std::find_if(items.begin(), items.end(), IdentMatch(id));
  if (it == items.end())
    return; // item not found

  if (it->sub) {
    it->sub->_parent_menu = 0;
    if (it->sub->_auto_delete) delete it->sub;
  }
  if (! it->separator) idset.reset(it->ident);
  items.erase(it);

  if (isVisible()) {
    updateSize();
    XClearArea(_app.getXDisplay(), _window,
               0, 0, _rect.width(), _rect.height(), True);
  } else {
    _size_dirty = true;
  }
}


void bt::Menu::removeIndex(unsigned int index) {
  ItemList::iterator it = items.begin();

  std::advance<ItemList::iterator, signed>(it, index);
  if (it == items.end())
    return; // item not found

  if (it->sub) {
    it->sub->_parent_menu = 0;
    if (it->sub->_auto_delete) delete it->sub;
  }
  if (! it->separator) idset.reset(it->ident);
  items.erase(it);

  if (isVisible()) {
    updateSize();
    XClearArea(_app.getXDisplay(), _window,
               0, 0, _rect.width(), _rect.height(), True);
  } else {
    _size_dirty = true;
  }
}


void bt::Menu::clear(void) {
  while (! items.empty())
    removeIndex(0);

  if (isVisible()) {
    updateSize();
    XClearArea(_app.getXDisplay(), _window,
               0, 0, _rect.width(), _rect.height(), True);
  } else {
    _size_dirty = true;
  }
}


void bt::Menu::showTitle(void) {
  _show_title = true;

    if (isVisible()) {
    updateSize();
    XClearArea(_app.getXDisplay(), _window,
               0, 0, _rect.width(), _rect.height(), True);
  } else {
    _size_dirty = true;
  }
}


void bt::Menu::hideTitle(void) {
  _show_title = false;

  if (isVisible()) {
    updateSize();
    XClearArea(_app.getXDisplay(), _window,
               0, 0, _rect.width(), _rect.height(), True);
  } else {
    _size_dirty = true;
  }
}


void bt::Menu::popup(int x, int y, bool centered) {
  _motion = 0;

  refresh();

  if (_size_dirty)
    updateSize();

  const ScreenInfo& screeninfo = _app.getDisplay().screenInfo(_screen);

  if (_show_title) {
    if (centered) {
      x -= _trect.width() / 2;
      if (x + _rect.width() > screeninfo.width())
        x = screeninfo.width() - _rect.width();

      y -= _trect.height() / 2;
      if (y + _rect.height() > screeninfo.height())
        y -= _frect.height() + (_trect.height() / 2);
    } else {
      if (x + _rect.width() > screeninfo.width())
        x -= _rect.width();

      y -= _trect.height();
      if (y + _rect.height() > screeninfo.height())
        y -= _frect.height();
    }
  } else {
    if (centered) {
      x -= _frect.width() / 2;
      if (x + _rect.width() > screeninfo.width())
        x = screeninfo.width() - _rect.width();
    } else {
      if (x + _rect.width() > screeninfo.width())
        x -= _rect.width();
    }

    if (y + _rect.height() > screeninfo.height())
      y -= _frect.height();
  }

  if (y < 0)
    y = 0;
  if (x < 0)
    x = 0;

  move(x, y);
  show();
}


void bt::Menu::move(int x, int y) {
  XMoveWindow(_app.getXDisplay(), _window, x, y);
  _rect.setPos(x, y);
}


void bt::Menu::show(void) {
  if (isVisible()) return;

  XMapRaised(_app.getXDisplay(), _window);
  XSync(_app.getXDisplay(), False);
  _app.openMenu(this);
  _visible = true;
  _pressed = _parent_menu ? _parent_menu->_pressed : false;
  _title_pressed = false;
}


void bt::Menu::hide(void) {
  if (! isVisible()) return;

  if (_active_submenu && _active_submenu->isVisible())
    _active_submenu->hide();
  _active_submenu = 0;
  _active_index = ~0u;

  if (showdelay.showmenu == this) showdelay.showmenu = 0;
  _timer.stop();

  ItemList::iterator it, end;
  for (it = items.begin(), end = items.end(); it != end; ++it) {
    if (it->active) {
      it->active = false;
      break;
    }
  }

  _app.closeMenu(this);
  XUnmapWindow(_app.getXDisplay(), _window);
  _visible = false;
  _pressed = false;
}


void bt::Menu::reconfigure(void) {
  ItemList::iterator it, end;
  for (it = items.begin(), end = items.end(); it != end; ++it) {
    if (it->sub) it->sub->reconfigure();
  }

  if (isVisible()) {
    updateSize();
    XClearArea(_app.getXDisplay(), _window,
               0, 0, _rect.width(), _rect.height(), True);
  } else {
    _size_dirty = true;
  }
}


void bt::Menu::updateSize(void) {
  MenuStyle* style = MenuStyle::get(_app, _screen, 0);

  if (_show_title) {
    _trect = style->titleRect(_title);
    _frect.setPos(0, _trect.height() - style->titleTexture().borderWidth());
  } else {
    _trect.setSize(0, 0);
    _frect.setPos(0, 0);
  }

  const ScreenInfo& screeninfo = _app.getDisplay().screenInfo(_screen);
  unsigned int max_item_w, col_h = 0u, max_col_h = 0u;
  unsigned int row = 0u, cols = 1u;
  max_item_w = std::max(20u, _trect.width());
  ItemList::iterator it, end;
  for (it= items.begin(), end = items.end(); it != end; ++it) {
    if (it->isSeparator()) {
      max_item_w = std::max(max_item_w, 20u);
      it->height = style->separatorHeight();
      col_h += it->height;
    } else {
      const Rect &rect = style->itemRect(*it);
      max_item_w = std::max(max_item_w, rect.width());
      it->height = rect.height();
      col_h += it->height;
    }

    ++row;

    if (col_h > (screeninfo.height() * 3 / 4)) {
      ++cols;
      row = 0;

      max_col_h = std::max(max_col_h, col_h);
      col_h = 0;
    }
  }

  // if we just changed to a new column, but have no items, then
  // remove the empty column
  if (cols > 1 && col_h == 0 && row == 0)
    --cols;
  max_col_h = std::max(std::max(max_col_h, col_h), style->frameMargin());

  // update rects
  _irect.setRect(style->frameMargin(), _frect.top() + style->frameMargin(),
                 std::max(_trect.width(), cols * max_item_w), max_col_h);
  _frect.setSize(_irect.width()  + (style->frameMargin() * 2),
                 _irect.height() + (style->frameMargin() * 2));
  _rect.setSize(_frect.width(), _frect.height());
  if (_show_title) {
    _trect.setWidth(std::max(_trect.width(), _frect.width()));
    _rect.setHeight(_rect.height() + _trect.height() -
                    style->titleTexture().borderWidth());
  }

  XResizeWindow(_app.getXDisplay(), _window, _rect.width(), _rect.height());

  // update pixmaps
  if (_show_title)
    _tpixmap = style->titlePixmap(_trect.width(), _trect.height(), _tpixmap);
  _fpixmap = style->framePixmap(_frect.width(), _frect.height(), _fpixmap);

  _itemw = max_item_w;
  _apixmap = style->activePixmap(_itemw, textHeight(style->frameFont()) +
                                 (style->itemMargin() * 2), _apixmap);

  _size_dirty = false;
}


void bt::Menu::buttonPressEvent(const XButtonEvent * const event) {
  if (! _rect.contains(event->x_root, event->y_root)) {
    hideAll();
    return;
  }

  _pressed = true;

  if (_trect.contains(event->x, event->y)) {
    _title_pressed = true;
    return;
  } else if (! _irect.contains(event->x, event->y)) {
    return;
  }

  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  unsigned int index = 0;
  ItemList::iterator it, end;
  for (it = items.begin(), end = items.end(); it != end; ++it, ++index) {
    r.setHeight(it->height);

    if (it->enabled && r.contains(event->x, event->y)) {
      if (! it->active)
        activateItem(r, *it);
      // ensure the submenu is visible
      showActiveSubmenu();
    }

    r.setY(r.y() + r.height());
    ++row;

    if (r.y() >= _irect.bottom()) {
      // next column
      ++col;
      row = 0;
      r.setPos(_irect.x() + (_itemw * col), _irect.y());
    }
  }
}


void bt::Menu::buttonReleaseEvent(const XButtonEvent * const event) {
  if (! _pressed && _motion < 10) return;

  _pressed = false;

  if (! _rect.contains(event->x_root, event->y_root)) {
    hideAll();
    return;
  }

  if (_title_pressed) {
    if (_trect.contains(event->x, event->y))
      titleClicked(event->button);
    _title_pressed = false;
    return;
  }

  bool do_hide = true;
  bool once = true;
  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  unsigned int index = 0;
  ItemList::iterator it, end;
  for (it = items.begin(), end = items.end(); it != end; ++index) {
    /*
      increment the iterator here, since the item could be removed
      below (which invalidates the iterator, and we will get a crash
      when we loop)
    */
    MenuItem &item = *it++;
    r.setHeight(item.height);

    if (item.enabled && r.contains(event->x, event->y) && once) {
      once = false;

      if (item.sub) {
        // clicked an item w/ a submenu, make sure the submenu is shown,
        // and keep the menu open
        do_hide = false;
        if (! item.active)
          activateItem(r, item);
        // ensure the submenu is visible
        showActiveSubmenu();
      }

      // clicked an enabled item
      itemClicked(item.ident, event->button);
    }

    r.setY(r.y() + r.height());
    ++row;

    if (r.y() >= _irect.bottom()) {
      // next column
      ++col;
      row = 0;
      r.setPos(_irect.x() + (_itemw * col), _irect.y());
    }
  }

  if (do_hide)
    hideAll();
}


void bt::Menu::motionNotifyEvent(const XMotionEvent * const event) {
  ++_motion;

  if (! _irect.contains(event->x, event->y))
    return;

  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  unsigned int index = 0;
  ItemList::iterator it, end;
  for (it = items.begin(), end = items.end(); it != end; ++it, ++index) {
    r.setHeight(it->height);

    if (r.contains(event->x, event->y)) {
      if (! it->active && it->enabled)
        activateItem(r, *it);
    } else if (it->active) {
      deactivateItem(r, *it);
    }

    r.setY(r.y() + r.height());
    ++row;

    if (r.y() >= _irect.bottom()) {
      // next column
      ++col;
      row = 0;
      r.setPos(_irect.x() + (_itemw * col), _irect.y());
    }
  }

  if (showdelay.showmenu && ! _timer.isTiming())
    _timer.start();
}


void bt::Menu::leaveNotifyEvent(const XCrossingEvent * const /*event*/) {
  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  ItemList::iterator it, end;
  for (it = items.begin(), end = items.end(); it != end; ++it) {
    r.setHeight(it->height);

    if (it->active && (! _active_submenu || it->sub != _active_submenu))
      deactivateItem(r, *it);

    r.setY(r.y() + r.height());
    ++row;

    if (r.y() >= _irect.bottom()) {
      // next column
      ++col;
      row = 0;
      r.setPos(_irect.x() + (_itemw * col), _irect.y());
    }
  }

  showActiveSubmenu();
}


void bt::Menu::exposeEvent(const XExposeEvent * const event) {
  MenuStyle* style = MenuStyle::get(_app, _screen, 0);
  Rect r(event->x, event->y, event->width, event->height), u;

  if (_show_title && r.intersects(_trect)) {
    u = r & _trect;

    Pen pen(_screen, style->titleTexture().color());
    if (_tpixmap) {
      XCopyArea(_app.getXDisplay(), _tpixmap, _window, pen.gc(),
                u.x(), u.y(), u.width(), u.height(), u.x(), u.y());
    } else {
      XFillRectangle(_app.getXDisplay(), _window, pen.gc(),
                     u.x(), u.y(), u.width(), u.height());
    }

    style->drawTitle(_window, _trect, _title);
  }

  if (r.intersects(_frect)) {
    u = r & _frect;

    Pen pen(_screen, style->frameTexture().color());
    if (_fpixmap) {
      XCopyArea(_app.getXDisplay(), _fpixmap, _window, pen.gc(),
                u.x() - _frect.x(), u.y() - _frect.y(),
                u.width(), u.height(), u.x(), u.y());
    } else {
      XFillRectangle(_app.getXDisplay(), _window, pen.gc(),
                     u.x(), u.y(), u.width(), u.height());
    }
  }

  if (! r.intersects(_irect))
    return;

  u = r & _irect;
  // only draw items that intersect with the needed update rect
  r.setRect(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  ItemList::const_iterator it, end;
  for (it = items.begin(), end = items.end(); it != end; ++it) {
    // note: we are reusing r from above, which is no longer needed now
    r.setHeight(it->height);

    if (r.intersects(u))
      style->drawItem(_window, r, *it, _apixmap);

    r.setY(r.y() + r.height());
    ++row;

    if (r.y() >= _irect.bottom()) {
      // next column
      ++col;
      row = 0;
      r.setPos(_irect.x() + (_itemw * col),  _irect.y());
    }
  }
}


void bt::Menu::keyPressEvent(const XKeyEvent * const event) {
  KeySym sym = XKeycodeToKeysym(_app.getXDisplay(), event->keycode, 0);
  switch (sym) {
  case XK_Escape: {
    hideAll();
    return;
  }

  case XK_Left: {
    if (_parent_menu)
      hide();
    return;
  }
  } // switch

  if (items.empty()) return;

  switch (sym) {
  case XK_Down: {
    ItemList::const_iterator anchor = items.begin(), end = items.end();
    if (_active_index != ~0u) {
      std::advance<ItemList::const_iterator, signed>(anchor, _active_index);

      // go one paste the current active index
      if (anchor != end && ! anchor->separator) ++anchor;
    }

    if (anchor == end) anchor = items.begin();

    ItemList::const_iterator it = std::find_if(anchor, end, InteractMatch());
    if (it != end) activateIndex(it->indx);
    break;
  }

  case XK_Up: {
    ItemList::const_reverse_iterator anchor = items.rbegin(),
                                        end = items.rend();
    if (_active_index != ~0u) {
      std::advance<ItemList::const_reverse_iterator, signed>(anchor, items.size() - _active_index - 1);

      // go one item past the current active index
      if (anchor != end && ! anchor->separator) ++anchor;
    }

    if (anchor == end) anchor = items.rbegin();

    ItemList::const_reverse_iterator it =
      std::find_if(anchor, end, InteractMatch());
    if (it != end) activateIndex(it->indx);
    break;
  }

  case XK_Home: {
    ItemList::const_iterator anchor = items.begin(), end = items.end();
    ItemList::const_iterator it = std::find_if(anchor, end, InteractMatch());
    if (it != end) activateIndex(it->indx);
    break;
  }

  case XK_End: {
    ItemList::const_reverse_iterator anchor = items.rbegin(),
                                        end = items.rend();
    ItemList::const_reverse_iterator it =
      std::find_if(anchor, end, InteractMatch());
    if (it != end) activateIndex(it->indx);
    break;
  }

  case XK_Right: {
    if (! _active_submenu) break;

    showActiveSubmenu();

    // activate the first item in the menu when shown with the keyboard
    ItemList::const_iterator anchor = _active_submenu->items.begin(),
                                end = _active_submenu->items.end();
    ItemList::const_iterator it = std::find_if(anchor, end, InteractMatch());
    if (it != end && _active_submenu->count() > 0)
      _active_submenu->activateIndex(it->indx);
    break;
  }

  case XK_Return:
  case XK_space: {
    if (_active_index == ~0u) break;

    ItemList::const_iterator it = items.begin(), end = items.end();
    it = std::find_if(it, end, IndexMatch(_active_index));
    if (it == end) break;

    /*
      the item could be removed in in the itemClicked call, so don't use
      the iterator after calling itemClicked
    */
    bool do_hide = (! it->sub);
    itemClicked(it->ident, 1);
    if (do_hide) hideAll();
    break;
  }
  } // switch
}


void bt::Menu::titleClicked(unsigned int button) {
  if (button == 3)
    hideAll();
}


void bt::Menu::itemClicked(unsigned int /*id*/, unsigned int /*button*/) {
}


void bt::Menu::hideAll(void) {
  if (_parent_menu && _parent_menu->isVisible())
    _parent_menu->hideAll();
  else
    hide();
}


unsigned int bt::Menu::verifyId(unsigned int id) {
  if (id != ~0u) {
    // request a specific id
    assert(id < 500);

    if (! idset.test(id)) {
      idset.set(id);
      return id;
    }


    fprintf(stderr, "Warning: bt::Menu::verifyId: id %d already used\n", id);
    abort();
  }

  // find the first available id
  id = 0;
  while (idset.test(id)) id++;
  idset.set(id);
  return id;
}


void bt::Menu::activateItem(const Rect &rect, MenuItem &item) {
  // mark new active item
  _active_index = item.indx;
  item.active = item.enabled;
  XClearArea(_app.getXDisplay(), _window,
             rect.x(), rect.y(), rect.width(), rect.height(), True);

  showdelay.showmenu = item.sub;
  _active_submenu = item.sub;
  if (! item.sub) return;

  item.sub->refresh();

  if (item.sub->_size_dirty)
    item.sub->updateSize();

  MenuStyle *style = MenuStyle::get(_app, _screen, 0);
  const ScreenInfo& screeninfo = _app.getDisplay().screenInfo(_screen);
  int px = _rect.x() + rect.x() + rect.width();
  int py = _rect.y() + rect.y() - style->frameMargin();
  bool left = false;

  if (_parent_menu && _parent_menu->isVisible() &&
      _parent_menu->_rect.x() > _rect.x())
    left = true;
  if (px + item.sub->_rect.width() > screeninfo.width() || left)
    px -= item.sub->_rect.width() + rect.width();
  if (px < 0) {
    if (left) // damn, lots of menus - move back to the right
      px = _rect.x() + rect.x() + rect.width();
    else
      px = 0;
  }

  if (_active_submenu->_show_title)
    py -=_active_submenu->_trect.height() -
         style->titleTexture().borderWidth();
  if (py + item.sub->_rect.height() > screeninfo.height())
    py -= item.sub->_irect.height() - rect.height();
  if (py < 0)
    py = 0;

  _active_submenu->move(px, py);
}


void bt::Menu::deactivateItem(const Rect &rect, MenuItem &item) {
  // clear old active item
  if (_active_index == item.indx) _active_index = ~0u;
  item.active = false;
  XClearArea(_app.getXDisplay(), _window,
             rect.x(), rect.y(), rect.width(), rect.height(), True);

  if (item.sub && item.sub->isVisible())
    item.sub->hide();
}


void bt::Menu::activateIndex(unsigned int index) {
  assert(index < items.size());

  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  ItemList::iterator it, end;
  for (it = items.begin(), end = items.end(); it != end; ++it) {
    r.setHeight(it->height);

    if (it->indx == index) {
      if (! it->active && it->enabled)
        activateItem(r, *it);
    } else if (it->active) {
      deactivateItem(r, *it);
    }

    r.setY(r.y() + r.height());
    ++row;

    if (r.y() >= _irect.bottom()) {
      // next column
      ++col;
      row = 0;
      r.setPos(_irect.x() + (_itemw * col), _irect.y());
    }
  }
}


void bt::Menu::showActiveSubmenu(void) {
  if (! _active_submenu) return;

  // active submenu, keep the menu item marked active and ensure
  // that the menu is visible
  if (! _active_submenu->isVisible())
    _active_submenu->show();
  showdelay.showmenu = 0;
  _timer.stop();
}

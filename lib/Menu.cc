// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Menu.cc for Blackbox - an X11 Window manager
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

#include "Menu.hh"
#include "Application.hh"
#include "Bitmap.hh"
#include "Display.hh"
#include "Pen.hh"
#include "PixmapCache.hh"
#include "Resource.hh"

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <stdio.h>
#include <assert.h>


bt::MenuStyle **bt::MenuStyle::styles = 0;


bt::MenuStyle *bt::MenuStyle::get(Application &app,
                                  unsigned int screen) {
  const size_t screen_count = app.display().screenCount();
  if (!styles) {
    styles = new MenuStyle*[screen_count];
    for (unsigned int i = 0; i < screen_count; ++i)
      styles[i] = 0;
  }
  // we need to remap screen N to 0 if there is only one
  const size_t index = (screen_count == 1) ? 0 : screen;
  if (!styles[index])
    styles[index] = new MenuStyle(app, screen);
  return styles[index];
}


bt::MenuStyle::MenuStyle(Application &app, unsigned int screen)
  : _app(app), _screen(screen) {
  title.alignment = AlignLeft;
  frame.alignment = AlignLeft;
  title_margin = frame_margin = item_indent = 1u;
}


void bt::MenuStyle::load(const Resource &resource) {
  // menu textures
  title.texture =
    textureResource(_app.display(), _screen, resource,
                    "menu.title", "Menu.Title", "black");
  frame.texture =
    textureResource(_app.display(), _screen, resource,
                    "menu.frame", "Menu.Frame", "white");
  active.texture =
    textureResource(_app.display(), _screen, resource,
                    "menu.active", "Menu.Active", "black");

  // non-texture colors
  title.foreground =
    Color::namedColor(_app.display(), _screen,
                      resource.read("menu.title.foregroundColor",
                                    "Menu.Title.ForegroundColor",
                                    "white"));
  title.text =
    Color::namedColor(_app.display(), _screen,
                      resource.read("menu.title.textColor",
                                    "Menu.Title.TextColor",
                                    "white"));
  frame.foreground =
    Color::namedColor(_app.display(), _screen,
                      resource.read("menu.frame.foregroundColor",
                                    "Menu.Frame.ForegroundColor",
                                    "black"));
  frame.text =
    Color::namedColor(_app.display(), _screen,
                      resource.read("menu.frame.textColor",
                                    "Menu.Frame.TextColor",
                                    "black"));
  frame.disabled =
    Color::namedColor(_app.display(), _screen,
                      resource.read("menu.frame.disabledColor",
                                    "Menu.Frame.DisabledColor",
                                    "black"));
  active.foreground =
    Color::namedColor(_app.display(), _screen,
                      resource.read("menu.active.foregroundColor",
                                    "Menu.Active.ForegroundColor",
                                    "white"));
  active.text =
    Color::namedColor(_app.display(), _screen,
                      resource.read("menu.active.textColor",
                                    "Menu.Active.TextColor",
                                    "white"));

  // fonts
  title.font.setFontName(resource.read("menu.title.font", "Menu.Title.Font"));
  frame.font.setFontName(resource.read("menu.frame.font", "Menu.Frame.Font"));

  const bt::Bitmap &arrow = bt::Bitmap::rightArrow(_screen);
  const bt::Bitmap &check = bt::Bitmap::checkMark(_screen);
  item_indent = std::max(check.width(), check.height());
  item_indent = std::max(item_indent, std::max(arrow.width(), arrow.height()));
  item_indent = std::max(item_indent, textHeight(_screen, frame.font));

  title.alignment =
    alignResource(resource, "menu.title.alignment",
                  "Menu.Title.Alignment");

  frame.alignment =
    alignResource(resource, "menu.frame.alignment",
                  "Menu.Frame.Alignment");

  std::string str;

  str = resource.read("menu.title.marginWidth", "Menu.Title.MarginWidth", "1");
  title_margin =
    static_cast<unsigned int>(std::max(strtol(str.c_str(), 0, 0), 0l));

  str = resource.read("menu.frame.marginWidth", "Menu.Frame.MarginWidth", "1");
  frame_margin =
    static_cast<unsigned int>(std::max(strtol(str.c_str(), 0, 0), 0l));
}


unsigned int bt::MenuStyle::separatorHeight(void) const {
  return
    (frame.texture.borderWidth() > 0 ? frame.texture.borderWidth() : 1) + 2;
}


unsigned int bt::MenuStyle::titleMargin(void) const {
  return title.texture.borderWidth() + title_margin;
}


unsigned int bt::MenuStyle::frameMargin(void) const {
  return frame.texture.borderWidth() + frame_margin;
}


unsigned int bt::MenuStyle::itemMargin(void) const {
  return active.texture.borderWidth() + (frame_margin == 0 ? 0 : 1);
}


bt::Rect bt::MenuStyle::titleRect(const ustring &text) const {
  const Rect &rect = textRect(_screen, title.font, text);
  return Rect(0, 0,
              rect.width()  + (titleMargin() * 2),
              rect.height() + (titleMargin() * 2));
}


bt::Rect bt::MenuStyle::itemRect(const MenuItem &item) const {
  const Rect &rect = textRect(_screen, frame.font, item.label());
  return Rect(0, 0,
              rect.width() + ((item_indent + itemMargin()) * 2),
              std::max(rect.height(), item_indent) + (itemMargin() * 2));
}


void bt::MenuStyle::drawTitle(Window window, const Rect &rect,
                              const ustring &text) const {
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
    XFillRectangle(pen.XDisplay(), window, pen.gc(),
                   r2.x(), r2.y() + 1, r2.width(),
                   (frame.texture.borderWidth() > 0 ?
                    frame.texture.borderWidth() : 1));
    return;
  }

  Pen fpen(_screen, (item.isEnabled() ? (item.isActive() ? active.foreground :
                                         frame.foreground) : frame.disabled));
  Pen tpen(_screen, (item.isEnabled() ? (item.isActive() ? active.text :
                                         frame.text) : frame.disabled));
  if (item.isActive() && item.isEnabled())
    drawTexture(_screen, active.texture, window, rect, rect, pixmap);
  drawText(frame.font, tpen, window, r2, frame.alignment, item.label());

  if (item.isChecked()) {
    drawBitmap(bt::Bitmap::checkMark(_screen), fpen, window,
               bt::Rect(rect.x(), rect.y(), rect.height(), rect.height()));
  }

  if (item.submenu()) {
    drawBitmap(bt::Bitmap::rightArrow(_screen), fpen, window,
               bt::Rect(rect.x() + rect.width() - rect.height(),
                        rect.y(), rect.height(), rect.height()));
  }
}


namespace bt {
  // internal helper classes

  class MenuDelay : public TimeoutHandler {
  public:
    Menu *showmenu;
    Menu *hidemenu;
    inline MenuDelay(void)
      : showmenu(0), hidemenu(0)
    { }
    inline void timeout(Timer *) {
      if (hidemenu)
        hidemenu->hide();
      if (showmenu)
        showmenu->show();
    }
  };

  class IdentMatch {
  public:
    unsigned int _id;
    inline IdentMatch(unsigned int id)
      : _id(id)
    { }
    inline bool operator()(const MenuItem &item) const
    { return item.id() == _id; }
  };

  class IndexMatch {
  public:
    unsigned int _index;
    inline IndexMatch(unsigned int index)
      : _index(index)
    { }
    inline bool operator()(const MenuItem &item) const
    { return item.index() == _index; }
  };

  class InteractMatch {
  public:
    inline bool operator()(const MenuItem &item) const
    { return item.isEnabled() && !item.isSeparator(); }
  };

} // namespace bt

static bt::MenuDelay menudelay;


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
    _timer(&_app, &menudelay),
    _parent_menu(0),
    _current_submenu(0),
    _active_submenu(0),
    _motion(0),
    _itemw(1),
    _active_index(~0u),
    _auto_delete(true),
    _pressed(false),
    _title_pressed(false),
    _size_dirty(true),
    _show_title(false),
    _visible(false)
{
  _id_bits.insert(_id_bits.begin(), 32, false);

  const ScreenInfo& screeninfo = _app.display().screenInfo(_screen);

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
    XCreateWindow(_app.XDisplay(), screeninfo.rootWindow(),
                  _rect.x(), _rect.y(), _rect.width(), _rect.height(), 0,
                  screeninfo.depth(), InputOutput,
                  screeninfo.visual(), mask, &attrib);
  _app.insertEventHandler(_window, this);

  _timer.setTimeout(200);
}


bt::Menu::~Menu(void) {
  hide();
  clear();

  bt::PixmapCache::release(_tpixmap);
  bt::PixmapCache::release(_fpixmap);
  bt::PixmapCache::release(_apixmap);
  _tpixmap = _fpixmap = _apixmap = 0ul;

  _app.removeEventHandler(_window);
  XDestroyWindow(_app.XDisplay(), _window);
}


void bt::Menu::invalidateSize(void) {
  if (isVisible()) {
    updateSize();
    updatePixmaps();
    XClearArea(_app.XDisplay(), _window,
               0, 0, _rect.width(), _rect.height(), True);
  } else {
    _size_dirty = true;
  }
}


unsigned int bt::Menu::insertItem(const MenuItem &item,
                                  unsigned int id, unsigned int index) {
  ItemList::iterator it;
  if (index == ~0u) {
    // append the new item
    index = _items.size();
    it = _items.end();
  } else {
    index = std::min(index, _items.size());
    it = _items.begin();
    std::advance<ItemList::iterator, signed>(it, index);
  }

  it = _items.insert(it, item);
  if (!item.separator) {
    id = verifyId(id);
    it->ident = id;
  }
  it->indx = index;

  // when inserting into the middle, update the index of all items
  // after the insertion point
  for (++it, ++index; it != _items.end(); ++it, ++index)
    it->indx = index;

  invalidateSize();

  return id;
}


unsigned int bt::Menu::insertItem(const ustring &label,
                                  unsigned int id, unsigned int index) {
  return insertItem(MenuItem(MenuItem::Normal, label), id, index);
}


unsigned int bt::Menu::insertItem(const ustring &label, Menu *submenu,
                                  unsigned int id, unsigned int index) {
  return insertItem(MenuItem(submenu, label), id, index);
}


void bt::Menu::insertSeparator(unsigned int index) {
  (void) insertItem(MenuItem(MenuItem::Separator), ~0u, index);
}


void bt::Menu::positionRect(Rect& r, int &row, int &col) {
  r.setY(r.y() + r.height());
  ++row;

  if (r.y() >= _irect.bottom()) {
    // next column
    ++col;
    row = 0;
    r.setPos(_irect.x() + (_itemw * col), _irect.y());
  }
}


// method assumes id exists and the items list has at least one item in it
bt::Menu::ItemList::iterator bt::Menu::findItem(unsigned int id, Rect& r) {
  int row = 0, col = 0;
  ItemList::iterator it, end;
  for (it = _items.begin(), end = _items.end(); it != end; ++it) {
    r.setHeight(it->height);

    if (it->id() == id)
      break;
  }

  if (it == end)
    return end;

  positionRect(r, row, col);
  return it;
}


void bt::Menu::changeItem(unsigned int id, const ustring &newlabel,
                          unsigned int newid) {
  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  ItemList::iterator it = findItem(id, r);
  if (it == _items.end())
    return;

  bt::MenuItem& item = *it;
  if (!item.isSeparator()) {
    if (item.lbl != newlabel) {
      // new label
      item.lbl = newlabel;
      invalidateSize();
    }

    if (newid != ~0u) {
      // change the id if necessary
      _id_bits[item.ident] = false;
      item.ident = verifyId(newid);
    }
  }
}


void bt::Menu::setItemEnabled(unsigned int id, bool enabled) {
  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  ItemList::iterator it = findItem(id, r);
  if (it == _items.end())
    return;

  bt::MenuItem& item = *it;
  // found the item, change the status and redraw if visible
  item.enabled = enabled;
  if (isVisible()) {
    XClearArea(_app.XDisplay(), _window,
               r.x(), r.y(), r.width(), r.height(), True);
  }
}


bool bt::Menu::isItemEnabled(unsigned int id) const {
  const ItemList::const_iterator &end = _items.end();
  const ItemList::const_iterator it =
    std::find_if(_items.begin(), end, IdentMatch(id));
  return (it != end && it->enabled);
}


void bt::Menu::setItemChecked(unsigned int id, bool checked) {
  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  ItemList::iterator it = findItem(id, r);
  if (it == _items.end())
    return;

  bt::MenuItem& item = *it;
  // found the item, change the status and redraw if visible
  item.checked = checked;
  if (isVisible()) {
    XClearArea(_app.XDisplay(), _window,
               r.x(), r.y(), r.width(), r.height(), True);
  }
}


bool bt::Menu::isItemChecked(unsigned int id) const {
  const ItemList::const_iterator &end = _items.end();
  const ItemList::const_iterator it =
    std::find_if(_items.begin(), end, IdentMatch(id));
  return (it != end && it->checked);
}


void bt::Menu::removeItemByIterator(ItemList::iterator& it) {
  if (it->sub) {
    if (it->sub->_auto_delete)
      delete it->sub;
  }

  if (!it->separator)
    _id_bits[it->ident] = false;
  _items.erase(it);

  invalidateSize();
}


void bt::Menu::removeItem(unsigned int id) {
  const ItemList::iterator &end = _items.end();
  ItemList::iterator it =
    std::find_if(_items.begin(), end, IdentMatch(id));
  if (it == end)
    return; // item not found
  removeItemByIterator(it);
}


void bt::Menu::removeIndex(unsigned int index) {
  ItemList::iterator it = _items.begin();
  std::advance<ItemList::iterator, signed>(it, index);
  if (it == _items.end())
    return; // item not found
  removeItemByIterator(it);
}


void bt::Menu::clear(void) {
  while (!_items.empty())
    removeIndex(0);
  invalidateSize();
}


void bt::Menu::showTitle(void) {
  _show_title = true;
  invalidateSize();
}


void bt::Menu::hideTitle(void) {
  _show_title = false;
  invalidateSize();
}


void bt::Menu::popup(int x, int y, bool centered)
{ popup(x, y, _app.display().screenInfo(_screen).rect(), centered); }


void bt::Menu::popup(int x, int y, const Rect &constraint, bool centered) {
  _motion = 0;

  refresh();

  if (_size_dirty)
    updateSize();

  bt::Rect u(x, y, _rect.width(), _rect.height());

  if (_show_title) {
    if (centered) {
      u.setPos(u.x() - _trect.width() / 2, u.y() - _trect.height() / 2);

      if (u.bottom() > constraint.bottom())
        u.setY(u.y() - _rect.height() + (_trect.height() / 2));
    } else {
      u.setY(u.y() - _trect.height());

      if (u.right() > constraint.right())
        u.setX(u.x() - _rect.width());
      if (u.bottom() > constraint.bottom())
        u.setY(u.y() - _rect.height());
    }
  } else {
    if (centered) {
      u.setX(u.x() - _frect.width() / 2);
    } else {
      if (u.right() > constraint.right())
        u.setX(u.x() - _rect.width());
      if (u.bottom() > constraint.bottom())
        u.setY(u.y() - _rect.height());
    }
  }

  if (u.right() > constraint.right())
    u.setX(constraint.right() - _rect.width() + 1);
  if (u.x() < constraint.x())
    u.setX(constraint.x());

  if (u.bottom() > constraint.bottom())
    u.setY(constraint.bottom() - _rect.height() + 1);
  if (u.y() < constraint.y())
    u.setY(constraint.y());

  move(u.x(), u.y());
  show();
}


void bt::Menu::move(int x, int y) {
  XMoveWindow(_app.XDisplay(), _window, x, y);
  _rect.setPos(x, y);
}


void bt::Menu::show(void) {
  if (isVisible())
    return;

  if (_parent_menu && _parent_menu->isVisible())
    _parent_menu->_current_submenu = this;

  // explicitly shown, no need to do it later
  if (menudelay.showmenu == this)
    menudelay.showmenu = 0;
  // don't hide this menu later if it is shown now
  if (menudelay.hidemenu == this)
    menudelay.hidemenu = 0;

  updatePixmaps();

  XMapRaised(_app.XDisplay(), _window);
  XSync(_app.XDisplay(), False);
  _app.openMenu(this);
  _visible = true;
  _pressed = _parent_menu ? _parent_menu->_pressed : false;
  _title_pressed = false;
}


void bt::Menu::hide(void) {
  if (!isVisible())
    return;

  if (_current_submenu && _current_submenu->isVisible())
    _current_submenu->hide();

  if (_parent_menu && _parent_menu->isVisible())
    _parent_menu->_current_submenu = 0;

  // don't show this menu later if it is hidden now
  if (menudelay.showmenu == this)
    menudelay.showmenu = 0;
  // explicitly hidden, no need to do it later
  if (menudelay.hidemenu == this)
    menudelay.hidemenu = 0;

  _active_index = ~0u;
  _active_submenu = 0;
  _parent_menu = 0;

  const ItemList::iterator &end = _items.end();
  ItemList::iterator it;
  for (it = _items.begin(); it != end; ++it) {
    if (it->active) {
      it->active = false;
      break;
    }
  }

  _app.closeMenu(this);
  XUnmapWindow(_app.XDisplay(), _window);
  _visible = false;
  _pressed = false;

  // release the pixmaps for this menu... menus are not visible 100% of the
  // time, so they should not use pixmap memory 100% of the time
  bt::PixmapCache::release(_tpixmap);
  bt::PixmapCache::release(_fpixmap);
  bt::PixmapCache::release(_apixmap);
  _tpixmap = _fpixmap = _apixmap = 0ul;
}


void bt::Menu::refresh(void)
{ }


void bt::Menu::reconfigure(void) {
  const ItemList::iterator &end = _items.end();
  ItemList::iterator it;
  for (it = _items.begin(); it != end; ++it) {
    if (it->sub)
      it->sub->reconfigure();
  }

  invalidateSize();
}


void bt::Menu::updateSize(void) {
  MenuStyle* style = MenuStyle::get(_app, _screen);

  if (_show_title) {
    _trect = style->titleRect(_title);
    _frect.setPos(0, _trect.height() - style->titleTexture().borderWidth());
  } else {
    _trect.setSize(0, 0);
    _frect.setPos(0, 0);
  }

  const ScreenInfo& screeninfo = _app.display().screenInfo(_screen);
  unsigned int col_h = 0u, max_col_h = 0u;
  unsigned int row = 0u, cols = 1u;
  _itemw = std::max(20u, _trect.width());
  const ItemList::iterator &end = _items.end();
  ItemList::iterator it;
  for (it= _items.begin(); it != end; ++it) {
    if (it->isSeparator()) {
      _itemw = std::max(_itemw, 20u);
      it->height = style->separatorHeight();
      col_h += it->height;
    } else {
      const Rect &rect = style->itemRect(*it);
      _itemw = std::max(_itemw, rect.width());
      it->height = rect.height();
      col_h += it->height;
    }

    ++row;

    if (col_h > (screeninfo.height() * 3u / 4u)) {
      ++cols;
      row = 0;

      max_col_h = std::max(max_col_h, col_h);
      col_h = 0;
    }
  }

  // if we just changed to a new column, but have no items, then
  // remove the empty column
  if (cols > 1u && col_h == 0u && row == 0u)
    --cols;

  max_col_h = std::max(std::max(max_col_h, col_h), style->frameMargin());

  // update rects
  _irect.setRect(style->frameMargin(), _frect.top() + style->frameMargin(),
                 std::max(_trect.width(), cols * _itemw), max_col_h);
  _frect.setSize(_irect.width()  + (style->frameMargin() * 2u),
                 _irect.height() + (style->frameMargin() * 2u));
  _rect.setSize(_frect.width(), _frect.height());
  if (_show_title) {
    _trect.setWidth(std::max(_trect.width(), _frect.width()));
    _rect.setHeight(_rect.height() + _trect.height() -
                    style->titleTexture().borderWidth());
  }

  XResizeWindow(_app.XDisplay(), _window, _rect.width(), _rect.height());
  _size_dirty = false;
}


void bt::Menu::updatePixmaps(void) {
  MenuStyle* style = MenuStyle::get(_app, _screen);

  // update pixmaps
  if (_show_title) {
    _tpixmap = PixmapCache::find(_screen, style->titleTexture(),
                                 _trect.width(), _trect.height(), _tpixmap);
  }
  _fpixmap = PixmapCache::find(_screen, style->frameTexture(),
                               _frect.width(), _frect.height(), _fpixmap);

  _apixmap =
    PixmapCache::find(_screen, style->activeTexture(), _itemw,
                      textHeight(_screen, style->frameFont()) +
                      (style->itemMargin() * 2), _apixmap);
}


void bt::Menu::buttonPressEvent(const XButtonEvent * const event) {
  if (!_rect.contains(event->x_root, event->y_root)) {
    hideAll();
    return;
  }

  _pressed = true;

  if (_trect.contains(event->x, event->y)) {
    _title_pressed = true;
    return;
  } else if (!_irect.contains(event->x, event->y)) {
    return;
  }

  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  unsigned int index = 0;
  const ItemList::iterator &end = _items.end();
  ItemList::iterator it;
  for (it = _items.begin(); it != end; ++it, ++index) {
    r.setHeight(it->height);

    if (!it->separator) {
      if (it->enabled && r.contains(event->x, event->y)) {
        if (!it->active)
          activateItem(r, *it);
        // ensure the submenu is visible
        showActiveSubmenu();
      }
    }

    positionRect(r, row, col);
  }
}


void bt::Menu::buttonReleaseEvent(const XButtonEvent * const event) {
  if (!_pressed && _motion < 10)
    return;

  _pressed = false;

  if (!_rect.contains(event->x_root, event->y_root)) {
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
  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  unsigned int index = 0;
  const ItemList::iterator &end = _items.end();
  ItemList::iterator it;
  for (it = _items.begin(); it != end; ++index) {
    /*
      increment the iterator here, since the item could be removed
      below (which invalidates the iterator, and we will get a crash
      when we loop)
    */
    MenuItem &item = *it++;
    r.setHeight(item.height);

    if (item.enabled && r.contains(event->x, event->y)) {
      if (item.separator) {
        do_hide = false;
      } else {
        if (item.sub) {
          // clicked an item w/ a submenu, make sure the submenu is shown,
          // and keep the menu open
          do_hide = false;
          if (!item.active)
            activateItem(r, item);
          // ensure the submenu is visible
          showActiveSubmenu();
        }

        // clicked an enabled item
        itemClicked(item.ident, event->button);
      }
      break;
    }

    positionRect(r, row, col);
  }

  if (do_hide)
    hideAll();
}


void bt::Menu::motionNotifyEvent(const XMotionEvent * const event) {
  ++_motion;

  if (!_irect.contains(event->x, event->y))
    return;

  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  unsigned int index = 0;
  const unsigned int old_active = _active_index;
  const ItemList::iterator &end = _items.end();
  ItemList::iterator it;
  for (it = _items.begin(); it != end; ++it, ++index) {
    r.setHeight(it->height);

    if (!it->separator) {
      if (r.contains(event->x, event->y)) {
        if (!it->active && it->enabled)
          activateItem(r, *it);
      } else if (it->active) {
        deactivateItem(r, *it, false);
      }
    }

    positionRect(r, row, col);
  }

  if (_active_index != old_active)
    _timer.start();
}


void bt::Menu::leaveNotifyEvent(const XCrossingEvent * const /*unused*/) {
  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  const ItemList::iterator &end = _items.end();
  ItemList::iterator it;
  for (it = _items.begin(); it != end; ++it) {
    r.setHeight(it->height);

    // deactivate the item unless its menu is visible
    if (!it->separator) {
      if (it->active && !(_current_submenu && _current_submenu == it->sub))
        deactivateItem(r, *it);
    }

    positionRect(r, row, col);
  }

  if (_timer.isTiming() && menudelay.hidemenu) {
    /*
      if the user has moved the mouse outside of the menu before the
      timer has fired (which hides the old menu and shows the new), we
      stop the timer and mark the "old" menu as active.

      this results in a really smart menu that allows you to move the
      mouse into submenus easily (even if you happen to move the mouse
      over other items in the menu)
    */
    _current_submenu = menudelay.hidemenu;
    menudelay.showmenu = 0;
    menudelay.hidemenu = 0;
    _timer.stop();

    r.setRect(_irect.x(), _irect.y(), _itemw, 0);
    row = col = 0;
    for (it = _items.begin(); it != end; ++it) {
      r.setHeight(it->height);

      if (!it->separator) {
        if (it->active && (!_current_submenu || it->sub != _current_submenu ||
                           !_current_submenu->isVisible()))
          deactivateItem(r, *it);
        else if (it->sub == _current_submenu)
          activateItem(r, *it);
      }

      positionRect(r, row, col);
    }
  }
}


void bt::Menu::exposeEvent(const XExposeEvent * const event) {
  MenuStyle* style = MenuStyle::get(_app, _screen);
  Rect r(event->x, event->y, event->width, event->height);

  if (_show_title && r.intersects(_trect)) {
    drawTexture(_screen, style->titleTexture(), _window,
                _trect, r & _trect, _tpixmap);
    style->drawTitle(_window, _trect, _title);
  }

  if (r.intersects(_frect)) {
    drawTexture(_screen, style->frameTexture(), _window,
                _frect, r & _frect, _fpixmap);
  }

  if (!r.intersects(_irect))
    return;

  Rect u = r & _irect;
  // only draw items that intersect with the needed update rect
  r.setRect(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  const ItemList::const_iterator &end = _items.end();
  ItemList::const_iterator it;
  for (it = _items.begin(); it != end; ++it) {
    // note: we are reusing r from above, which is no longer needed now
    r.setHeight(it->height);

    if (r.intersects(u))
      style->drawItem(_window, r, *it, _apixmap);

    positionRect(r, row, col);
  }
}


void bt::Menu::activateSubmenu(void) {
  if (!_active_submenu)
    return;

  showActiveSubmenu();

  // activate the first item in the menu when shown with the keyboard
  const ItemList::const_iterator &end = _current_submenu->_items.end();
  ItemList::const_iterator it = _current_submenu->_items.begin();
  it = std::find_if(it, end, InteractMatch());
  if (it != end && _current_submenu->count() > 0)
    _current_submenu->activateIndex(it->indx);
}


void bt::Menu::keyPressEvent(const XKeyEvent * const event) {
  KeySym sym = XKeycodeToKeysym(_app.XDisplay(), event->keycode, 0);
  switch (sym) {
  case XK_Escape: {
    hide();
    return;
  }

  case XK_Left: {
    if (_parent_menu && _parent_menu->isVisible())
      hide();
    return;
  }
  } // switch

  if (_items.empty())
    return;

  switch (sym) {
  case XK_Down: {
    const ItemList::const_iterator &end = _items.end();
    ItemList::const_iterator anchor = _items.begin();
    if (_active_index != ~0u) {
      std::advance<ItemList::const_iterator, signed>(anchor, _active_index);

      // go one paste the current active index
      if (anchor != end && !anchor->separator)
        ++anchor;
    }

    if (anchor == end)
      anchor = _items.begin();

    ItemList::const_iterator it = std::find_if(anchor, end, InteractMatch());
    if (it != end)
      activateIndex(it->indx);
    break;
  }

  case XK_Up: {
    ItemList::const_reverse_iterator anchor = _items.rbegin();
    const ItemList::const_reverse_iterator &end = _items.rend();
    if (_active_index != ~0u) {
      std::advance<ItemList::const_reverse_iterator, signed>
        (anchor, _items.size() - _active_index - 1);

      // go one item past the current active index
      if (anchor != end && !anchor->separator)
        ++anchor;
    }

    if (anchor == end) anchor = _items.rbegin();

    ItemList::const_reverse_iterator it =
      std::find_if(anchor, end, InteractMatch());
    if (it != end)
      activateIndex(it->indx);
    break;
  }

  case XK_Home: {
    const ItemList::const_iterator &end = _items.end();
    ItemList::const_iterator it = _items.begin();
    it = std::find_if(it, end, InteractMatch());
    if (it != end)
      activateIndex(it->indx);
    break;
  }

  case XK_End: {
    const ItemList::const_reverse_iterator &end = _items.rend();
    ItemList::const_reverse_iterator it = _items.rbegin();
    it = std::find_if(it, end, InteractMatch());
    if (it != end)
      activateIndex(it->indx);
    break;
  }

  case XK_Right: {
    activateSubmenu();
    break;
  }

  case XK_Return:
  case XK_space: {
    if (_active_index == ~0u)
      break;

    const ItemList::const_iterator &end = _items.end();
    ItemList::const_iterator it = _items.begin();
    it = std::find_if(it, end, IndexMatch(_active_index));
    if (it == end)
      break;

    if (!it->separator) {
      /*
        the item could be removed in the itemClicked call, so don't use
        the iterator after calling itemClicked
      */
      if (it->sub) {
        activateSubmenu();
      } else {
        itemClicked(it->ident, 1);
        hideAll();
      }
    }
    break;
  }
  } // switch
}


void bt::Menu::titleClicked(unsigned int button) {
  if (button == 3)
    hideAll();
}


void bt::Menu::itemClicked(unsigned int /*id*/, unsigned int /*button*/)
{ }


void bt::Menu::hideAll(void) {
  if (_parent_menu && _parent_menu->isVisible())
    _parent_menu->hideAll();
  else
    hide();
}


unsigned int bt::Menu::verifyId(unsigned int id) {
  if (id != ~0u) {
    // request a specific id

    // expand if necessary
    while (id >= _id_bits.size())
      _id_bits.insert(_id_bits.end(), _id_bits.size(), false);

    if (!_id_bits[id]) {
      _id_bits[id] = true;
      return id;
    }

    fprintf(stderr, "Error: bt::Menu::verifyId: id %u already used\n", id);
    abort();
  }

  std::vector<bool>::iterator it =
    std::find(_id_bits.begin(), _id_bits.end(), false);
  if (it == _id_bits.end()) {
    // no free bits... expand
    _id_bits.insert(_id_bits.end(), _id_bits.size(), false);
    it = std::find(_id_bits.begin(), _id_bits.end(), false);
    assert(it != _id_bits.end());
  }

  *it = true;
  return static_cast<int>(it - _id_bits.begin());
}


void bt::Menu::activateItem(const Rect &rect, MenuItem &item) {
  // mark new active item
  _active_index = item.indx;
  _active_submenu = item.sub;
  if (_active_submenu)
    _active_submenu->_parent_menu = this;

  item.active = item.enabled;
  XClearArea(_app.XDisplay(), _window,
             rect.x(), rect.y(), rect.width(), rect.height(), True);

  menudelay.showmenu = item.sub;
  if (menudelay.hidemenu == item.sub)
    menudelay.hidemenu = 0;
  if (!item.sub || item.sub->isVisible())
    return;

  item.sub->refresh();

  if (item.sub->_size_dirty)
    item.sub->updateSize();

  MenuStyle *style = MenuStyle::get(_app, _screen);
  const ScreenInfo& screeninfo = _app.display().screenInfo(_screen);
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

  if (item.sub->_show_title)
    py -= item.sub->_trect.height() -
         style->titleTexture().borderWidth();
  if (py + item.sub->_rect.height() > screeninfo.height())
    py -= item.sub->_irect.height() - rect.height();
  if (py < 0)
    py = 0;

  item.sub->move(px, py);
}


void bt::Menu::deactivateItem(const Rect &rect, MenuItem &item,
                              bool hide_submenu) {
  // clear old active item
  if (_active_index == item.indx) {
    _active_index = ~0u;
    _active_submenu = 0;
  }

  item.active = false;
  XClearArea(_app.XDisplay(), _window,
             rect.x(), rect.y(), rect.width(), rect.height(), True);

  if (item.sub) {
    if (menudelay.showmenu == item.sub)
      menudelay.showmenu = 0;
    if (item.sub->isVisible()) {
      if (hide_submenu)
        item.sub->hide();
      else
        menudelay.hidemenu = item.sub;
    }
  }
}


void bt::Menu::activateIndex(unsigned int index) {
  assert(index < _items.size());

  Rect r(_irect.x(), _irect.y(), _itemw, 0);
  int row = 0, col = 0;
  const ItemList::iterator &end = _items.end();
  ItemList::iterator it;
  for (it = _items.begin(); it != end; ++it) {
    r.setHeight(it->height);

    if (!it->separator) {
      if (it->indx == index) {
        if (!it->active && it->enabled)
          activateItem(r, *it);
      } else if (it->active) {
        deactivateItem(r, *it);
      }
    }

    positionRect(r, row, col);
  }
}


void bt::Menu::showActiveSubmenu(void) {
  if (!_active_submenu)
    return;

  if (menudelay.hidemenu)
    menudelay.hidemenu->hide();
  menudelay.hidemenu = 0;

  // active submenu, keep the menu item marked active and ensure
  // that the menu is visible
  if (!_active_submenu->isVisible())
    _active_submenu->show();
  menudelay.showmenu = 0;
  _timer.stop();
}

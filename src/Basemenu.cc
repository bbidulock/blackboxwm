// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Basemenu.cc for Blackbox - an X11 Window manager
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "Basemenu.hh"
#include "BaseDisplay.hh"
#include "Color.hh"
#include "GCCache.hh"
#include "Screen.hh"
#include "blackbox.hh"
#include "i18n.hh"

#include <stdio.h>


// new basemenu popup

Basemenu::Basemenu( int scr )
  : Widget( scr, Popup ),
    title_pixmap( 0 ), items_pixmap( 0 ), highlight_pixmap( 0 ),
    parent_menu( 0 ), current_submenu( 0 ),
    motion( 0 ), rows( 0 ), cols( 0 ), itemw( 0 ), indent( 0 ),
    show_title( false ), size_dirty( true ),
    pressed( false ), title_pressed( false )
{
}

Basemenu::~Basemenu()
{
}

void Basemenu::drawTitle()
{
  BStyle *style = Blackbox::instance()->screen( screen() )->style();

  int dx = 1;
  int l;
  if (i18n->multibyte()) {
    XRectangle ink, logical;
    XmbTextExtents( style->menuTitleFontSet(), title().c_str(), title().length(),
                    &ink, &logical );
    l = logical.width;
  } else
    l = XTextWidth( style->menuTitleFont(), title().c_str(), title().length() );
  l += 2;

  switch ( style->menuTitleJustify() ) {
  case BStyle::Left:
    dx += indent;
    break;

  case BStyle::Right:
    dx += title_rect.width() - l - indent;
    break;

  case BStyle::Center:
    dx += ( title_rect.width() - l )/ 2;
    break;
  }

  BGCCache::Item &gc = BGCCache::instance()->find( style->menuTitleTextColor(),
                                                   style->menuTitleFont() );
  if ( i18n->multibyte() )
    XmbDrawString( *BaseDisplay::instance(), windowID(), style->menuTitleFontSet(),
                   gc.gc(), title_rect.x() + dx, title_rect.y() +
                   ( style->bevelWidth() -
                     style->menuTitleFontSetExtents()->max_ink_extent.y ),
                   title().c_str(), title().length() );
  else
    XDrawString( *BaseDisplay::instance(), windowID(), gc.gc(),
                 title_rect.x() + dx, title_rect.y() +
                 (style->menuTitleFont()->ascent + style->bevelWidth()),
                 title().c_str(), title().length() );
  BGCCache::instance()->release( gc );
}

void Basemenu::drawItem( const Rect &r, const Item &item )
{
  BaseDisplay *display = BaseDisplay::instance();
  BGCCache *cache = BGCCache::instance();
  BStyle *style = Blackbox::instance()->screen( screen() )->style();

  if ( item.isSeparator() ) {
    BGCCache::Item &gc = cache->find( style->menuTextColor() );
    XFillRectangle( *display, windowID(), gc.gc(),
                    r.x() + indent / 2, r.y(), r.width() - indent, r.height() );
    cache->release( gc );
    return;
  }

  int dx = 1;
  int l;
  if ( i18n->multibyte() ) {
    XRectangle ink, logical;
    XmbTextExtents( style->menuFontSet(), item.label().c_str(),
                    item.label().length(), &ink, &logical );
    l = logical.width;
  } else
    l = XTextWidth( style->menuFont(), item.label().c_str(),
                    item.label().length() );
  l += 2;

  switch ( style->menuJustify() ) {
  case BStyle::Left:
    dx += indent;
    break;

  case BStyle::Right:
    dx += r.width() - l - indent;
    break;

  case BStyle::Center:
    dx += ( r.width() - l ) / 2;
    break;
  }

  if ( item.isActive() ) {
    BGCCache::Item &gc = cache->find( style->menuHighlight().color() );
    if (  highlight_pixmap )
      XCopyArea( *display, highlight_pixmap, windowID(), gc.gc(),
                 0, 0, r.width(), r.height(), r.x(), r.y() );
    else
      XFillRectangle( *display, windowID(), gc.gc(),
                      r.x(), r.y(), r.width(), r.height() );
    cache->release( gc );
  }

  BGCCache::Item &gc =
    cache->find( ( item.isActive() ? style->menuHighlightTextColor() :
                   item.isEnabled() ? style->menuTextColor() :
                   style->menuDisabledTextColor() ), style->menuFont() );

  if ( i18n->multibyte() )
    XmbDrawString( *display, windowID(), style->menuFontSet(), gc.gc(),
                   r.x() + dx, r.y() + ( style->bevelWidth() -
                                         style->menuFontSetExtents()->max_ink_extent.y ),
                   item.label().c_str(), item.label().length() );
  else
    XDrawString( *display, windowID(), gc.gc(),
                 r.x() + dx, r.y() + ( style->menuFont()->ascent +
                                       style->bevelWidth() ),
                 item.label().c_str(), item.label().length() );

  if ( item.isChecked() ) {
    // draw check mark
    int cx = r.x() + ( indent - 7 ) / 2;
    int cy = r.y() + ( indent - 7 ) / 2;
    XSetClipMask( *display, gc.gc(), style->checkBitmap() );
    XSetClipOrigin( *display, gc.gc(), cx, cy );
    XFillRectangle( *display, windowID(), gc.gc(), cx, cy, 7, 7 );
    XSetClipOrigin( *display, gc.gc(), 0, 0 );
    XSetClipMask( *display, gc.gc(), None );
  }

  if ( item.submenu() ) {
    // draw submenu arrow
    int ax = r.x() + r.width() - indent + ( indent - 7 ) / 2;
    int ay = r.y() + ( indent - 7 ) / 2;
    XSetClipMask( *display, gc.gc(), style->arrowBitmap() );
    XSetClipOrigin( *display, gc.gc(), ax, ay );
    XFillRectangle( *display, windowID(), gc.gc(), ax, ay, 7, 7 );
    XSetClipOrigin( *display, gc.gc(), 0, 0 );
    XSetClipMask( *display, gc.gc(), None );
  }

  cache->release( gc );
}

void Basemenu::updateSize()
{
  int w = 0, h = 0, iw;
  int index, titleh, itemh, colh = 0, maxcolh = 0;

  BScreen *scr = Blackbox::instance()->screen( screen() );
  BStyle *style = scr->style();
  if (i18n->multibyte()) {
    maxcolh = itemh = style->menuFontSetExtents()->max_ink_extent.height +
              style->bevelWidth() * 2;
    titleh = style->menuTitleFontSetExtents()->max_ink_extent.height +
             style->bevelWidth() * 2;
  } else {
    maxcolh = itemh = style->menuFont()->ascent + style->menuFont()->descent +
              style->bevelWidth() * 2;
    titleh = style->menuTitleFont()->ascent + style->menuTitleFont()->descent +
             style->bevelWidth() * 2;
  }

  indent = itemh;
  h = style->borderWidth();

  Items::iterator it = items.begin();
  if ( show_title ) {
    if ( it == items.end() ) {
      // internal error
      fprintf( stderr, "Basemenu: cannot update size, internal error\n" );
      return;
    }

    Item &item = *it++;

    if ( i18n->multibyte() ) {
      XRectangle ink, logical;
      XmbTextExtents( style->menuTitleFontSet(), item.label().c_str(),
                      item.label().length(), &ink, &logical );
      logical.width += style->bevelWidth() * 2;
      iw = int( logical.width );
    } else
      iw = XTextWidth( style->menuTitleFont(), item.label().c_str(),
                       item.label().length() ) + style->bevelWidth() * 2;

    iw += indent + indent;
    item.height = titleh;
    item.idx = -1;

    w = std::max( w, iw );
    h += item.height;
    h += style->borderWidth();
  }

  rows = 0;
  cols = 1;
  index = 0;

  while ( it != items.end() ) {
    Item &item = *it++;

    item.idx = index++;

    if ( item.isSeparator() ) {
      iw = 80;
      item.height = style->borderWidth() > 0 ? style->borderWidth() : 1;
    } else {
      if ( i18n->multibyte() ) {
        XRectangle ink, logical;
        XmbTextExtents( style->menuFontSet(), item.label().c_str(),
                        item.label().length(), &ink, &logical );
        logical.width += style->bevelWidth() * 2;
        iw = std::max( w, int( logical.width ) );
      } else
        iw = XTextWidth( style->menuFont(), item.label().c_str(),
                         item.label().length() ) + style->bevelWidth() * 2;

      iw += indent + indent;
      item.height = itemh;
    }

    w = std::max( w, iw );
    colh += item.height;
    rows++;

    if ( colh > scr->height() * 3 / 4 ) {
      maxcolh = std::max( maxcolh, colh );
      colh = 0;
      cols++;
      rows = 0;
    }
  }

  if ( w < 80 )
    w = 80;

  maxcolh = std::max( maxcolh, colh );
  maxcolh += style->bevelWidth() * 2;
  h += maxcolh + style->borderWidth();

  itemw = w;
  w *= cols;
  w += style->bevelWidth() * 2;
  w += style->borderWidth() * 2;

  if ( show_title ) {
    Rect tr( style->borderWidth(), style->borderWidth(),
             w - style->borderWidth() * 2, titleh );
    Rect ir( style->borderWidth(), titleh + style->borderWidth() * 2,
             w - style->borderWidth() * 2, h - titleh - style->borderWidth() * 3 );
    if ( tr != title_rect || ! title_pixmap ) {
      title_rect = tr;
      title_pixmap = style->menuTitle().render( title_rect.size(), title_pixmap );
    }
    if ( ir != items_rect || ! items_pixmap ) {
      items_rect = ir;
      // itemw = items_rect.width() / cols;
      items_pixmap = style->menu().render( items_rect.size(), items_pixmap );
      highlight_pixmap = style->menuHighlight().render( Size( itemw, itemh ),
                                                        highlight_pixmap );
    }
  } else {
    Rect ir( style->borderWidth(), style->borderWidth(),
             w - style->borderWidth() * 2, h - style->borderWidth() * 2 );
    if ( ir != items_rect || ! items_pixmap ) {
      items_rect = ir;
      // itemw = items_rect.width() / cols;
      items_pixmap = style->menu().render( items_rect.size(), items_pixmap );
      highlight_pixmap = style->menuHighlight().render( Size( itemw, itemh ),
                                                        highlight_pixmap );
    }
  }

  Size sz( w, h );
  if ( sz != size() )
    resize( sz );
  size_dirty = false;
}

void Basemenu::reconfigure()
{
  title_rect = items_rect = Rect(0, 0, 1, 1);
  size_dirty = true;
  updateSize();
  if ( isVisible() )
    XClearArea( *BaseDisplay::instance(), windowID(),
                x(), y(), width(), height(), True );
}

void Basemenu::popup( int x, int y, bool centerOnTitle )
{
  motion = 0;

  // let dynamic menus do any needed inserts/changes/removes before we call
  // updateSize()
  refresh();

  if ( size_dirty )
    updateSize();

  BScreen *scr = Blackbox::instance()->screen( screen() );
  if ( show_title ) {
    if ( centerOnTitle ) {
      // if the title is visible, center it around the popup point
      Point p = Point( x, y ) - title_rect.pos() -
                Point( title_rect.width() / 2, title_rect.height() / 2 );
      x = p.x();
      y = p.y();
      if ( y + height() > scr->height() )
        y -= items_rect.height() + title_rect.height() / 2 +
             scr->style()->borderWidth() * 2;
      if ( y < 0 )
        y = 0;
      if ( x + width() > scr->width() )
        x = scr->width() - width();
      if ( x < 0 )
        x = 0;
    } else {
      y -= title_rect.y() + title_rect.height();
      if ( y + height() > scr->height() )
        y -= items_rect.height();
      if ( y < 0 )
        y = 0;
      if ( x + width() > scr->width() )
        x -= items_rect.width();
      if ( x < 0 )
        x = 0;
    }
  } else {
    if ( y + height() > scr->height() )
      y -= height();
    if ( y < 0 )
      y = 0;
    if ( x + width() > scr->width() )
      x -= width();
    if ( x < 0 )
      x = 0;
  }

  move( x, y );
  show();
}

void Basemenu::popup( const Point &p, bool centerOnTitle )
{
  popup( p.x(), p.y(), centerOnTitle );
}

void Basemenu::hide()
{
  if ( current_submenu && current_submenu->isVisible() )
    current_submenu->hide();
  current_submenu = 0;

  Items::iterator it = items.begin();
  if ( show_title ) {
    if ( it == items.end() ){
      // internal error
      fprintf( stderr, "Basemenu: cannot track active item, internal error\n" );
      return;
    }

    it++;
  }

  Rect r;
  int row = 0, col = 0;
  BStyle *style = Blackbox::instance()->screen( screen() )->style();
  int x = items_rect.x() + style->bevelWidth();
  int y = items_rect.y() + style->bevelWidth();
  while ( it != items.end() ) {
    Item &item = (*it++);
    r.setRect( x, y, itemw, item.height );

    if ( item.active ) {
      item.active = false;
    }

    y += item.height;
    row++;

    if ( y > items_rect.y() + items_rect.height() ) {
      // next column
      col++;
      row = 0;
      y = items_rect.y() + style->bevelWidth();
      x = items_rect.x() + style->bevelWidth() + itemw * col;
    }
  }

  Widget::hide();
}

void Basemenu::hideAll()
{
  if ( parent_menu && parent_menu->isVisible() )
    parent_menu->hideAll();
  else
    hide();
}

int Basemenu::insert( const string &label, const Item &item, int index )
{
  Items::iterator it;

  if ( index >= 0 ) {
    it = items.begin();

    if ( show_title ) {
      if ( it == items.end() ) {
        // internal error
        fprintf( stderr, "Basemenu: cannot insert item, internal error\n" );
        return -1;
      }

      it++;
    }

    int i = 0;
    while ( i++ < index && it != items.end() )
      it++;
    index = i;
  } else {
    // append
    it = items.end();
    index = items.size();
  }

  it = items.insert( it, item );
  (*it).lbl = label;
  if ( (*it).submenu() )
    (*it).submenu()->parent_menu = this;

  if ( isVisible() ) {
    updateSize();
    XClearArea( *BaseDisplay::instance(), windowID(),
                x(), y(), width(), height(), True );
  } else
    size_dirty = true;

  return index;
}

void Basemenu::change( int index,
			const string &label,
			const Item &item )
{
  Items::iterator it = items.begin();

  if ( show_title ) {
    if ( it == items.end() ) {
      // internal error
      fprintf( stderr, "Basemenu: cannot change item, internal error\n" );
      return;
    }

    it++;
  }

  int i = 0;
  while ( i++ < index ) {
    if ( it == items.end() ) {
      // index is out of range
      fprintf( stderr, "Basemenu: cannot change item %d, index out of range\n",
               index );
      return;
    }

    it++;
  }

  if ( ! item.def )
    (*it) = item;
  (*it).lbl = label;
  if ( (*it).submenu() )
    (*it).submenu()->parent_menu = this;

  if ( isVisible() ) {
    updateSize();
    XClearArea( *BaseDisplay::instance(), windowID(),
                x(), y(), width(), height(), True );
  } else
    size_dirty = true;
}

void Basemenu::remove( int index )
{
  Items::iterator it = items.begin();

  if ( show_title ) {
    if ( it == items.end() ) {
      // index is out of range
      fprintf( stderr, "Basemenu: cannot remove item, internal error\n" );
      return;
    }

    it++;
  }

  int i = 0;
  while ( i++ < index ) {
    if ( it == items.end() ) {
      // index is out of range
      fprintf( stderr, "Basemenu: cannot remove item %d, index out of range\n",
               index );
      return;
    }

    it++;
  }

  if ( it == items.end() ) {
    // index is out of range
    fprintf( stderr, "Basemenu: cannot remove item %d, index out of range\n",
             index );
    return;
  }

  if ( (*it).submenu() )
    (*it).submenu()->parent_menu = 0;
  items.erase( it );

  if ( isVisible() ) {
    updateSize();
    XClearArea( *BaseDisplay::instance(), windowID(),
                x(), y(), width(), height(), True );
  } else
    size_dirty = true;
}

void Basemenu::clear()
{
  while ( count() > 0 )
    remove(0);
}

bool Basemenu::isItemEnabled( int index ) const
{
  Items::const_iterator it = items.begin();

  if ( show_title ) {
    if ( it == items.end() ) {
      // internal error
      fprintf( stderr, "Basemenu: cannot find item, internal error\n" );
      return false;
    }

    it++;
  }

  int i = 0;
  while ( i++ < index ) {
    if ( it == items.end() ) {
      // index is out of range
      fprintf( stderr, "Basemenu: cannot find item %d, index out of range\n",
               index );
      return false;
    }

    it++;
  }

  return (*it).isEnabled();
}

void Basemenu::setItemEnabled( int index, bool enabled )
{
  Items::iterator it = items.begin();

  if ( show_title ) {
    if ( it == items.end() ) {
      // internal error
      fprintf( stderr, "Basemenu: cannot find item, internal error\n" );
      return;
    }

    it++;
  }

  Rect r;
  int i = 0;
  int row = 0, col = 0;
  BStyle *style = Blackbox::instance()->screen( screen() )->style();
  int x = items_rect.x() + style->bevelWidth();
  int y = items_rect.y() + style->bevelWidth();
  while ( i++ < index ) {
    if ( it == items.end() ) {
      // index is out of range
      fprintf( stderr, "Basemenu: cannot find item %d, index out of range\n",
               index );
      return;
    }

    Item &item = *it++;
    r.setRect( x, y, itemw, item.height );
    y += item.height;
    row++;

    if ( y > items_rect.y() + items_rect.height() ) {
      // next column
      col++;
      row = 0;
      y = items_rect.y() + style->bevelWidth();
      x = items_rect.x() + style->bevelWidth() + itemw * col;
    }
  }

  if ( it == items.end() ) {
    // index is out of range
    fprintf( stderr, "Basemenu: cannot find item %d, index out of range\n",
             index );
    return;
  }

  (*it).enable = enabled;
  if ( isVisible() ) {
     XClearArea( *BaseDisplay::instance(), windowID(),
                  r.x(), r.y(), r.width(), r.height(), True );
  }
}

bool Basemenu::isItemChecked( int index ) const
{
  Items::const_iterator it = items.begin();

  if ( show_title ) {
    if ( it == items.end() ) {
      // internal error
      fprintf( stderr, "Basemenu: cannot find item, internal error\n" );
      return false;
    }

    it++;
  }

  int i = 0;
  while ( i++ < index ) {
    if ( it == items.end() ) {
      // index is out of range
      fprintf( stderr, "Basemenu: cannot find item %d, index out of range\n",
               index );
      return false;
    }

    it++;
  }

  if ( it == items.end() ) {
    // index is out of range
    fprintf( stderr, "Basemenu: cannot find item %d, index out of range\n",
             index );
    return false;
  }

  return (*it).isChecked();
}

void Basemenu::setItemChecked( int index, bool check )
{
  Items::iterator it = items.begin();

  if ( show_title ) {
    if ( it == items.end() ) {
      // internal error
      fprintf( stderr, "Basemenu: cannot find item, internal error\n" );
      return;
    }

    it++;
  }

  Rect r;
  int i = 0;
  int row = 0, col = 0;
  BStyle *style = Blackbox::instance()->screen( screen() )->style();
  int x = items_rect.x() + style->bevelWidth();
  int y = items_rect.y() + style->bevelWidth();
  while ( i++ < index ) {
    if ( it == items.end() ) {
      // index is out of range
      fprintf( stderr, "Basemenu: cannot find item %d, index out of range\n",
               index );
      return;
    }

    Item &item = *it++;
    r.setRect( x, y, itemw, item.height );
    y += item.height;
    row++;

    if ( y > items_rect.y() + items_rect.height() ) {
      // next column
      col++;
      row = 0;
      y = items_rect.y() + style->bevelWidth();
      x = items_rect.x() + style->bevelWidth() + itemw * col;
    }
  }

  if ( it == items.end() ) {
    // index is out of range
    fprintf( stderr, "Basemenu: cannot find item %d, index out of range\n",
             index );
    return;
  }

  (*it).checked = check;
  if ( isVisible() ) {
    XClearArea( *BaseDisplay::instance(), windowID(),
                r.x(), r.y(), r.width(), r.height(), True );
  }
}

void Basemenu::showTitle()
{
  if ( show_title )
    return;

  Item item;
  item.lbl = title();
  item.title = true;
  items.insert( items.begin(), item );

  show_title = true;

  if ( isVisible() ) {
    updateSize();
    XClearArea( *BaseDisplay::instance(), windowID(),
                x(), y(), width(), height(), True );
  } else
    size_dirty = true;
}

void Basemenu::hideTitle()
{
  if ( ! show_title )
    return;

  items.erase( items.begin() );

  show_title = false;

  if ( isVisible() ) {
    updateSize();
    XClearArea( *BaseDisplay::instance(), windowID(),
                x(), y(), width(), height(), True );
  } else
    size_dirty = true;
}

void Basemenu::setActiveItem( const Rect &r, Item &item )
{
  if ( item.active )
    return;

  item.active = item.isEnabled();
  drawItem( r, item );
  showSubmenu( r, item );
}

void Basemenu::showSubmenu( const Rect &r, const Item &item )
{
  if ( current_submenu && current_submenu->isVisible() )
    current_submenu->hide();
  current_submenu = 0;

  if ( ! item.submenu() )
    return;

  // let dynamic menus do any needed inserts/changes/removes before we call
  // updateSize()
  item.submenu()->refresh();

  // position the submenu
  if ( item.submenu()->size_dirty )
    item.submenu()->updateSize();

  BScreen *scr = Blackbox::instance()->screen( screen() );

  int px = x() + r.x() + r.width();
  int py = y() + r.y() - scr->style()->borderWidth() - scr->style()->bevelWidth();
  bool on_left = false;

  if ( parent_menu && parent_menu->isVisible() && parent_menu->x() > x() )
    on_left = true;
  // move the submenu to the left side of the menu, where there is hopefully more space
  if ( px + item.submenu()->width() > scr->width() || on_left )
    px -= item.submenu()->width() + r.width();
  if ( px < 0 ) {
    if ( on_left )
      // wow, lots of nested menus - move submenus back to the right side
      px = x() + r.x() + r.width();
    else
      px = 0;
  }

  if ( item.submenu()->show_title ) {
    py -= item.submenu()->title_rect.y() + item.submenu()->title_rect.height();
    if ( py + item.submenu()->height() > scr->height() )
      py -= item.submenu()->items_rect.height() - r.height();
    if ( py < 0 )
      py = 0;
  } else {
    if ( py + item.submenu()->height() > scr->height() )
      py -= item.submenu()->items_rect.height() - r.height();
    if ( py < 0 )
      py = 0;
  }

  // show the submenu
  current_submenu = item.submenu();
  current_submenu->move( px, py );
  current_submenu->show();
}

void Basemenu::buttonPressEvent( XEvent *e )
{
  if ( ! rect().contains( Point( e->xbutton.x_root, e->xbutton.y_root ) ) ) {
    hideAll();
    return;
  }

  pressed = true;

  Point p( e->xbutton.x, e->xbutton.y );

  if ( title_rect.contains( p ) ) {
    title_pressed = true;
    return;
  } else if (! items_rect.contains( p ) )
    return;

  Items::iterator it = items.begin();
  if ( show_title ) {
    if ( it == items.end() ){
      // internal error
      fprintf( stderr, "Basemenu: cannot track active item, internal error\n" );
      return;
    }

    it++;
  }

  Rect r;
  int row = 0, col = 0;
  BStyle *style = Blackbox::instance()->screen( screen() )->style();
  int x = items_rect.x() + style->bevelWidth();
  int y = items_rect.y() + style->bevelWidth();
  while ( it != items.end() ) {
    Item &item = (*it++);
    r.setRect( x, y, itemw, item.height );

    if ( r.contains( p ) ) {
      setActiveItem( r, item );
    } else if ( item.active ) {
      item.active = false;
      XClearArea( *BaseDisplay::instance(), windowID(),
                  r.x(), r.y(), r.width(), r.height(), True );
    }

    y += item.height;
    row++;

    if ( y > items_rect.y() + items_rect.height() ) {
      // next column
      col++;
      row = 0;
      y = items_rect.y() + style->bevelWidth();
      x = items_rect.x() + style->bevelWidth() + itemw * col;
    }
  }
}

void Basemenu::buttonReleaseEvent( XEvent *e )
{
  if ( ! pressed && motion < 5 )
    return;

  pressed = false;

  if ( ! rect().contains( Point( e->xbutton.x_root, e->xbutton.y_root ) ) ) {
    hideAll();
    return;
  }

  Point p( e->xbutton.x, e->xbutton.y );

  if ( title_rect.contains( p ) && title_pressed ) {
    titleClicked( p - title_rect.pos(), e->xbutton.button );
    title_pressed = false;
    return;
  }

  bool do_hide = true;

  title_pressed = false;

  Items::iterator it = items.begin();
  if ( show_title ) {
    if ( it == items.end() ){
      // internal error
      fprintf( stderr, "Basemenu: cannot track active item, internal error\n" );
      return;
    }

    it++;
  }

  Rect r;
  bool once = true;
  int row = 0, col = 0;
  BStyle *style = Blackbox::instance()->screen( screen() )->style();
  int x = items_rect.x() + style->bevelWidth();
  int y = items_rect.y() + style->bevelWidth();
  while ( it != items.end() ) {
    Item &item = (*it++);
    r.setRect( x, y, itemw, item.height );

    if ( r.contains( p ) && once ) {
      setActiveItem( r, item );
      if ( item.isEnabled() )
        itemClicked( p - items_rect.pos(), item, e->xbutton.button );
      if (  item.submenu() )
        do_hide = false;
      once = false;
    } else if ( item.active ) {
      item.active = false;
      XClearArea( *BaseDisplay::instance(), windowID(),
                  r.x(), r.y(), r.width(), r.height(), True );
    }

    y += item.height;
    row++;

    if ( y > items_rect.y() + items_rect.height() ) {
      // next column
      col++;
      row = 0;
      y = items_rect.y() + style->bevelWidth();
      x = items_rect.x() + style->bevelWidth() + itemw * col;
    }
  }

  if ( do_hide )
    hideAll();
}

void Basemenu::pointerMotionEvent( XEvent *e )
{
  motion++;

  Point gp( e->xmotion.x_root, e->xmotion.y_root );
  if ( ! rect().contains( gp ) )
    return;

  Point p( e->xmotion.x, e->xmotion.y );
  if ( ! items_rect.contains( p ) && ( show_title && ! title_rect.contains( p ) ) )
    return;

  Items::iterator it = items.begin();
  if ( show_title ) {
    if ( it == items.end() ){
      // internal error
      fprintf( stderr, "Basemenu: cannot track active item, internal error\n" );
      return;
    }

    it++;
  }

  Rect r;
  int row = 0, col = 0;
  BStyle *style = Blackbox::instance()->screen( screen() )->style();
  int x = items_rect.x() + style->bevelWidth();
  int y = items_rect.y() + style->bevelWidth();
  while ( it != items.end() ) {
    Item &item = (*it++);
    r.setRect( x, y, itemw, item.height );

    if ( r.contains( p ) ) {
      if ( current_submenu && item.submenu() != current_submenu ) {
        current_submenu->hide();
        current_submenu = 0;
      }

      title_pressed = false;
      setActiveItem( r, item );
    } else if ( item.active ) {
      if ( current_submenu && item.submenu() == current_submenu ) {
        current_submenu->hide();
        current_submenu = 0;
      }

      item.active = false;
      XClearArea( *BaseDisplay::instance(), windowID(),
                  r.x(), r.y(), r.width(), r.height(), True );
    }

    y += item.height;
    row++;

    if ( y > items_rect.y() + items_rect.height() ) {
      // next column
      col++;
      row = 0;
      y = items_rect.y() + style->bevelWidth();
      x = items_rect.x() + style->bevelWidth() + itemw * col;
    }
  }
}

void Basemenu::leaveEvent( XEvent * )
{
  Items::iterator it = items.begin();
  if ( show_title ) {
    if ( it == items.end() ){
      // internal error
      fprintf( stderr, "Basemenu: cannot track active item, internal error\n" );
      return;
    }

    it++;
  }

  Rect r;
  int row = 0, col = 0;
  BStyle *style = Blackbox::instance()->screen( screen() )->style();
  int x = items_rect.x() + style->bevelWidth();
  int y = items_rect.y() + style->bevelWidth();
  while ( it != items.end() ) {
    Item &item = (*it++);
    r.setRect( x, y, itemw, item.height );

    if ( item.active && ( ! current_submenu ||
                          current_submenu != item.submenu() ) ) {
      item.active = false;
      XClearArea( *BaseDisplay::instance(), windowID(),
                  r.x(), r.y(), r.width(), r.height(), True );
    }

    y += item.height;
    row++;

    if ( y > items_rect.y() + items_rect.height() ) {
      // next column
      col++;
      row = 0;
      y = items_rect.y() + style->bevelWidth();
      x = items_rect.x() + style->bevelWidth() + itemw * col;
    }
  }
}

void Basemenu::exposeEvent( XEvent *e )
{
  Rect todo( e->xexpose.x, e->xexpose.y, e->xexpose.width, e->xexpose.height );
  BStyle *style = Blackbox::instance()->screen( screen() )->style();
  BaseDisplay *display = BaseDisplay::instance();
  BGCCache *cache = BGCCache::instance();

  if ( style->borderWidth() ) {
    // draw the borders of the menu if they need updating
    XRectangle xrects[5];
    int num = 0;
    if ( todo.y() < style->borderWidth() ) {
      // top line
      xrects[num].x = style->borderWidth();
      xrects[num].y = 0;
      xrects[num].width = width() - style->borderWidth() * 2;
      xrects[num].height = style->borderWidth();
      num++;
    }
    if ( todo.y() + todo.height() > height() - style->borderWidth() ) {
      xrects[num].x = style->borderWidth();
      xrects[num].y = height() - style->borderWidth();
      xrects[num].width = width() - style->borderWidth() * 2;
      xrects[num].height = style->borderWidth();
      num++;
    }
    if ( todo.x() < style->borderWidth() ) {
      xrects[num].x = 0;
      xrects[num].y = 0;
      xrects[num].width = style->borderWidth();
      xrects[num].height = height();
      num++;
    }
    if ( todo.x() + todo.width() > width() - style->borderWidth() ) {
      xrects[num].x = width() - style->borderWidth();
      xrects[num].y = 0;
      xrects[num].width = style->borderWidth();
      xrects[num].height = height();
      num++;
    }
    if ( show_title &&
         ( todo.y() < title_rect.y() + title_rect.height() ||
           todo.y() + todo.height() > title_rect.y() + title_rect.height() ) ) {
      xrects[num].x = style->borderWidth();
      xrects[num].y = title_rect.y() + title_rect.height();
      xrects[num].width = width() - style->borderWidth() * 2;
      xrects[num].height = style->borderWidth();
      num++;
    }
    if ( num > 0 ) {
      BGCCache::Item &bgc = cache->find( style->borderColor() );
      XFillRectangles(*display, windowID(), bgc.gc(), xrects, num );
      cache->release( bgc );
    }
  }

  BGCCache::Item &tgc = cache->find( style->menuTitle().color() ),
                 &igc = cache->find( style->menu().color() );

  if ( show_title && todo.intersects( title_rect ) ) {
    Rect up = title_rect & todo;
    if ( style->menuTitle().texture() == ( BImage_Solid | BImage_Flat ) )
      XFillRectangle( *display, windowID(), tgc.gc(),
                      up.x(), up.y(), up.width(), up.height() );
    else if ( title_pixmap )
      XCopyArea( *display, title_pixmap, windowID(), tgc.gc(),
                 up.x() - title_rect.x(), up.y() - title_rect.y(),
                 up.width(), up.height(), up.x(), up.y() );
    drawTitle();
  }
  if ( todo.intersects( items_rect ) ) {
    Rect up = items_rect & todo;
    if ( style->menu().texture() == ( BImage_Solid | BImage_Flat ) )
      XFillRectangle( *BaseDisplay::instance(), windowID(), igc.gc(),
                      up.x(), up.y(), up.width(), up.height() );
    else if ( items_pixmap )
      XCopyArea( *display, items_pixmap, windowID(), igc.gc(),
                 up.x() - items_rect.x(), up.y() - items_rect.y(),
                 up.width(), up.height(), up.x(), up.y() );

    Items::const_iterator it = items.begin();
    if ( show_title ) {
      if ( it == items.end() ) {
        // internal error
        fprintf( stderr, "Basemenu: cannot draw menu items, internal error\n" );
        cache->release( tgc );
        cache->release( igc );
        return;
      }
      it++;
    }

    // only draw items that intersect with the needed update rect
    Rect r;
    int row = 0, col = 0;
    int x = items_rect.x() + style->bevelWidth();
    int y = items_rect.y() + style->bevelWidth();
    while ( it != items.end() ) {
      const Item &item = (*it++);
      r.setRect( x, y, itemw, item.height );

      if ( r.intersects( todo ) )
        drawItem( r, item );

      y += item.height;
      row++;

      if ( y > items_rect.y() + items_rect.height() ) {
        // next column
        col++;
        row = 0;
        y = items_rect.y() + style->bevelWidth();
        x = items_rect.x() + style->bevelWidth() + itemw * col;
      }
    }
  }

  cache->release( tgc );
  cache->release( igc );
}

void Basemenu::titleClicked( const Point &, int button )
{
  if ( button == 3 )
    hideAll();
}

void Basemenu::itemClicked( const Point &, const Item &, int )
{
}

// Basemenu.cc for Blackbox - an X11 Window manager
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
    break;

  case BStyle::Right:
    dx += title_rect.width() - l;
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
                   ( 1 - style->menuTitleFontSetExtents()->max_ink_extent.y ),
                   title().c_str(), title().length() );
  else
    XDrawString( *BaseDisplay::instance(), windowID(), gc.gc(),
                 title_rect.x() + dx, title_rect.y() +
                 (style->menuTitleFont()->ascent + 1),
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
    XDrawLine( *display, windowID(), gc.gc(),
               r.x() + indent / 2, r.y(), r.x() + r.width() - indent / 2, r.y() );
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
    dx += r.width() - l;
    break;

  case BStyle::Center:
    dx += ( r.width() - l ) / 2;
    break;
  }

  if ( item.isActive() ) {
    BGCCache::Item gc = cache->find( style->menuHighlight().color() );
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
                   r.x() + dx, r.y() +
                   ( 1 - style->menuFontSetExtents()->max_ink_extent.y ),
                   item.label().c_str(), item.label().length() );
  else
    XDrawString( *display, windowID(), gc.gc(),
                 r.x() + dx, r.y() + ( style->menuFont()->ascent + 1 ),
                 item.label().c_str(), item.label().length() );

  if ( item.isChecked() ) {
    // draw check mark
    int cx = r.x() + ( indent - 7 ) / 2;
    int cy = r.y() + ( indent - 7 ) / 2;
    XSetClipMask( *display, gc.gc(), style->menuCheckBitmap() );
    XSetClipOrigin( *display, gc.gc(), cx, cy );
    XFillRectangle( *display, windowID(), gc.gc(), cx, cy, 7, 7 );
    XSetClipOrigin( *display, gc.gc(), 0, 0 );
    XSetClipMask( *display, gc.gc(), None );
  }

  if ( item.submenu() ) {
    // draw submenu arrow
    int ax = r.x() + r.width() - indent + ( indent - 7 ) / 2;
    int ay = r.y() + ( indent - 7 ) / 2;
    XSetClipMask( *display, gc.gc(), style->menuArrowBitmap() );
    XSetClipOrigin( *display, gc.gc(), ax, ay );
    XFillRectangle( *display, windowID(), gc.gc(), ax, ay, 7, 7 );
    XSetClipOrigin( *display, gc.gc(), 0, 0 );
    XSetClipMask( *display, gc.gc(), None );
  }

  cache->release( gc );
}

void Basemenu::updateSize()
{
  int w = 1, h = 1, iw;
  int index, titleh, itemh, colh = 0, maxcolh = 0;

  BScreen *scr = Blackbox::instance()->screen( screen() );
  BStyle *style = scr->style();
  if (i18n->multibyte()) {
    itemh = style->menuFontSetExtents()->max_ink_extent.height + 2;
    titleh = style->menuTitleFontSetExtents()->max_ink_extent.height + 2;
  } else {
    itemh = style->menuFont()->ascent + style->menuFont()->descent + 2;
    titleh = style->menuTitleFont()->ascent + style->menuTitleFont()->descent + 2;
  }

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
      logical.width += 2;
      iw = int( logical.width );
    } else
      iw = XTextWidth( style->menuTitleFont(), item.label().c_str(),
                       item.label().length() ) + 2;

    item.height = titleh;
    item.idx = -1;

    w = max( w, iw );
    h += item.height;
    h += 1;
  }

  rows = 0;
  cols = 1;
  index = 0;

  while ( it != items.end() ) {
    Item &item = *it++;

    item.idx = index++;

    if ( item.isSeparator() ) {
      iw = 80;
      item.height = 1;
    } else {
      if ( i18n->multibyte() ) {
        XRectangle ink, logical;
        XmbTextExtents( style->menuFontSet(), item.label().c_str(),
                        item.label().length(), &ink, &logical );
        logical.width += 2;
        iw = max( w, int( logical.width ) );
      } else
        iw = XTextWidth( style->menuFont(), item.label().c_str(),
                         item.label().length() ) + 2;

      iw += itemh + itemh;
      item.height = itemh;
    }

    w = max( w, iw );
    colh += item.height;
    rows++;

    if ( colh > scr->height() * 3 / 4 ) {
      maxcolh = max( maxcolh, colh );
      colh = 0;
      cols++;
      rows = 0;
    }
  }

  maxcolh = max( maxcolh, colh );
  h += maxcolh + 1;
  w *= cols;

  if ( w < 80 )
    w = 80;
  if ( h < 6 )
    h = 6;

  if ( show_title ) {
    Rect tr( 1, 1, w - 2, titleh );
    Rect ir( 1, titleh + 2, w - 2, h - titleh - 3 );
    if ( tr != title_rect || ! title_pixmap ) {
      title_rect = tr;
      title_pixmap = style->menuTitle().render( title_rect.size(), title_pixmap );
    }
    if ( ir != items_rect || ! items_pixmap ) {
      items_rect = ir;
      itemw = items_rect.width() / cols;
      items_pixmap = style->menu().render( items_rect.size(), items_pixmap );
      highlight_pixmap = style->menuHighlight().render( Size( itemw, itemh ),
                                                        highlight_pixmap );
    }
  } else {
    Rect ir( 1, 1, w - 2, h - 2 );
    if ( ir != items_rect || ! items_pixmap ) {
      items_rect = ir;
      itemw = items_rect.width() / cols;
      items_pixmap = style->menu().render( items_rect.size(), items_pixmap );
      highlight_pixmap = style->menuHighlight().render( Size( itemw, itemh ),
                                                        highlight_pixmap );
    }
  }

  indent = itemh;

  Size sz( w, h );
  if ( sz != size() )
    resize( sz );
  size_dirty = false;
}

void Basemenu::reconfigure()
{
  size_dirty = true;
  updateSize();
  if ( isVisible() )
    XClearArea( *BaseDisplay::instance(), windowID(),
                x(), y(), width(), height(), True );
}

void Basemenu::popup( int x, int y, bool centerOnTitle )
{
  motion = 0;
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
        y -= items_rect.height() + title_rect.height() / 2;
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
      y -= items_rect.height();
    if ( y < 0 )
      y = 0;
    if ( x + width() > scr->width() )
      x -= items_rect.width();
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
  if ( current_submenu )
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
  int x = items_rect.x(), y = items_rect.y();
  while ( it != items.end() ) {
    Item &item = (*it++);
    r.setRect( x, y, itemw, item.height );

    if ( item.active ) {
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
      y = items_rect.y();
      x = items_rect.x() + itemw * col;
    }
  }

  Widget::hide();
}

void Basemenu::hideAll()
{
  if ( parent_menu && parent_menu->isVisible() )
    parent_menu->hide();
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
			const Item &item = Item::Default )
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
  int x = items_rect.x(), y = items_rect.y();
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
      y = items_rect.y();
      x = items_rect.x() + itemw * col;
    }
  }

  if ( it == items.end() ) {
    // index is out of range
    fprintf( stderr, "Basemenu: cannot find item %d, index out of range\n",
             index );
    return;
  }

  (*it).enable = enabled;
  if ( isVisible() )
    XClearArea( *BaseDisplay::instance(), windowID(),
                r.x(), r.y(), r.width(), r.height(), True );
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
  int x = items_rect.x(), y = items_rect.y();
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
      y = items_rect.y();
      x = items_rect.x() + itemw * col;
    }
  }

  if ( it == items.end() ) {
    // index is out of range
    fprintf( stderr, "Basemenu: cannot find item %d, index out of range\n",
             index );
    return;
  }

  (*it).checked = check;
  if ( isVisible() )
    XClearArea( *BaseDisplay::instance(), windowID(),
                r.x(), r.y(), r.width(), r.height(), True );
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
  if ( current_submenu )
    current_submenu->hide();
  current_submenu = 0;

  if ( ! item.submenu() )
    return;

  // show the submenu
  if ( item.submenu()->size_dirty )
    item.submenu()->updateSize();

  Point p = pos() + Point( r.x() + r.width(),
                           r.y() - ( item.submenu()->show_title ?
                                     item.submenu()->title_rect.height() + 1  : 1 ) );
  current_submenu = item.submenu();
  current_submenu->move( p );
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
  int x = items_rect.x(), y = items_rect.y();
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
      y = items_rect.y();
      x = items_rect.x() + itemw * col;
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
  int row = 0, col = 0;
  int x = items_rect.x(), y = items_rect.y();
  while ( it != items.end() ) {
    Item &item = (*it++);
    r.setRect( x, y, itemw, item.height );

    if ( r.contains( p ) ) {
      setActiveItem( r, item );
      if ( item.isEnabled() )
        itemClicked( p - items_rect.pos(), item, e->xbutton.button );
      if (  item.submenu() )
        do_hide = false;
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
      y = items_rect.y();
      x = items_rect.x() + itemw * col;
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
  int x = items_rect.x(), y = items_rect.y();
  while ( it != items.end() ) {
    Item &item = (*it++);
    r.setRect( x, y, itemw, item.height );

    if ( r.contains( p ) ) {
      title_pressed = false;
      setActiveItem( r, item );
    } else if ( item.active ) {
      item.active = false;
      XClearArea( *BaseDisplay::instance(), windowID(),
                  r.x(), r.y(), r.width(), r.height(), True );
      if ( current_submenu && item.submenu() == current_submenu ) {
        current_submenu->hide();
        current_submenu = 0;
      }
    }

    y += item.height;
    row++;

    if ( y > items_rect.y() + items_rect.height() ) {
      // next column
      col++;
      row = 0;
      y = items_rect.y();
      x = items_rect.x() + itemw * col;
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
  int x = items_rect.x(), y = items_rect.y();
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
      y = items_rect.y();
      x = items_rect.x() + itemw * col;
    }
  }
}

void Basemenu::exposeEvent( XEvent *e )
{
  Rect todo( e->xexpose.x, e->xexpose.y, e->xexpose.width, e->xexpose.height );
  BStyle *style = Blackbox::instance()->screen( screen() )->style();
  BaseDisplay *display = BaseDisplay::instance();
  BGCCache *cache = BGCCache::instance();
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
        return;
      }
      it++;
    }

    // only draw items that intersect with the needed update rect
    Rect r;
    int row = 0, col = 0;
    int x = items_rect.x(), y = items_rect.y();
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
        y = items_rect.y();
        x = items_rect.x() + itemw * col;
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

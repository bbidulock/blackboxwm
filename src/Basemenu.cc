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
#include "Style.hh"
#include "i18n.hh"

#include <stdio.h>

#define check_width 7
#define check_height 7
static unsigned char check_bits[] = {
   0x40, 0x60, 0x71, 0x3b, 0x1f, 0x0e, 0x04};


// new basemenu popup

Basemenu2::Basemenu2( int scr )
    : Widget( scr, Popup ),
      title_pixmap( 0 ), items_pixmap( 0 ), highlight_pixmap( 0 ),
      parent_menu( 0 ), current_submenu( 0 ),
      motion( 0 ), rows( 0 ), cols( 0 ), itemw( 0 ), indent( 0 ),
      show_title( false ), size_dirty( true ),
      pressed( false ), title_pressed( false )
{

}

Basemenu2::~Basemenu2()
{
}

void Basemenu2::drawTitle()
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
    case BScreen::RightJustify:
	dx += title_rect.width() - l;
	break;

    case BScreen::CenterJustify:
	dx += ( title_rect.width() - l )/ 2;
	break;
    }

    BGCCache::Item &gc = BGCCache::instance()->find( style->t_text, style->t_font );
    if ( i18n->multibyte() )
	XmbDrawString( *BaseDisplay::instance(), windowID(), style->t_fontset, gc.gc(),
		       title_rect.x() + dx, title_rect.y() +
		       ( 1 - style->t_fontset_extents->max_ink_extent.y ),
		       title().c_str(), title().length() );
    else
	XDrawString( *BaseDisplay::instance(), windowID(), gc.gc(),
		     title_rect.x() + dx, title_rect.y() + (style->t_font->ascent + 1),
		     title().c_str(), title().length() );
    BGCCache::instance()->release( gc );
}

void Basemenu2::drawItem( const Rect &r, const Item &item )
{
    BaseDisplay *display = BaseDisplay::instance();
    BGCCache *cache = BGCCache::instance();
    MenuStyle *style = Blackbox::instance()->screen( screen() )->getMenuStyle();

    if ( item.isSeparator() ) {
	BGCCache::Item &gc = cache->find( style->f_text );
	XDrawLine( *display, windowID(), gc.gc(),
		   r.x() + indent / 2, r.y(), r.x() + r.width() - indent / 2, r.y() );
	cache->release( gc );
	return;
    }

    int dx = 1;
    int l;
    if ( i18n->multibyte() ) {
	XRectangle ink, logical;
	XmbTextExtents( style->f_fontset, item.label().c_str(), item.label().length(),
			&ink, &logical );
	l = logical.width;
    } else
	l = XTextWidth( style->f_font, item.label().c_str(), item.label().length() );
    l += 2;

    switch ( style->f_justify ) {
    case BScreen::LeftJustify:
	dx += indent;
	break;

    case BScreen::RightJustify:
	dx += r.width() - l;
	break;

    case BScreen::CenterJustify:
	dx += ( r.width() - l ) / 2;
	break;
    }

    if ( item.isActive() ) {
	BGCCache::Item gc = cache->find( style->hilite.color() );
	if (  highlight_pixmap )
	    XCopyArea( *display, highlight_pixmap, windowID(), gc.gc(),
		       0, 0, r.width(), r.height(), r.x(), r.y() );
	else
	    XFillRectangle( *display, windowID(), gc.gc(),
			    r.x(), r.y(), r.width(), r.height() );
	cache->release( gc );
    }

    BGCCache::Item &gc = cache->find( ( item.isActive() ? style->h_text :
					item.isEnabled() ? style->f_text :
					style->d_text ), style->f_font );
    if ( i18n->multibyte() )
	XmbDrawString( *display, windowID(), style->f_fontset, gc.gc(),
		       r.x() + dx, r.y() +
		       ( 1 - style->f_fontset_extents->max_ink_extent.y ),
		       item.label().c_str(), item.label().length() );
    else
	XDrawString( *display, windowID(), gc.gc(),
		     r.x() + dx, r.y() + ( style->f_font->ascent + 1 ),
		     item.label().c_str(), item.label().length() );
    cache->release( gc );
}

void Basemenu2::updateSize()
{
    int w = 1, h = 1, iw;
    int index, titleh, itemh, colh = 0, maxcolh = 0;

    BScreen *scr = Blackbox::instance()->screen( screen() );
    MenuStyle *style = scr->getMenuStyle();
    if (i18n->multibyte()) {
	itemh = style->f_fontset_extents->max_ink_extent.height + 2;
	titleh = style->t_fontset_extents->max_ink_extent.height + 2;
    } else {
	itemh = style->f_font->ascent + style->f_font->descent + 2;
	titleh = style->t_font->ascent + style->t_font->descent + 2;
    }

    slist<Item>::iterator it = items.begin();
    if ( show_title ) {
	if ( it == items.end() ) {
	    // internal error
	    fprintf( stderr, "Basemenu2: cannot update size, internal error\n" );
	    return;
	}

	Item &item = *it++;

	if ( i18n->multibyte() ) {
	    XRectangle ink, logical;
	    XmbTextExtents( style->t_fontset, item.label().c_str(),
			    item.label().length(), &ink, &logical );
	    logical.width += 2;
	    iw = int( logical.width );
	} else
	    iw = XTextWidth( style->t_font, item.label().c_str(),
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
		XmbTextExtents( style->f_fontset, item.label().c_str(),
				item.label().length(), &ink, &logical );
		logical.width += 2;
		iw = max( w, int( logical.width ) );
	    } else
		iw = XTextWidth( style->f_font, item.label().c_str(),
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
	    title_pixmap = style->title.render( title_rect.size(), title_pixmap );
	}
	if ( ir != items_rect || ! items_pixmap ) {
	    items_rect = ir;
	    itemw = items_rect.width() / cols;
	    items_pixmap = style->frame.render( items_rect.size(), items_pixmap );
	    highlight_pixmap = style->hilite.render( Size( itemw, itemh ),
						     highlight_pixmap );
	}
    } else {
	Rect ir( 1, 1, w - 2, h - 2 );
	if ( ir != items_rect || ! items_pixmap ) {
	    items_rect = ir;
	    itemw = items_rect.width() / cols;
	    items_pixmap = style->frame.render( items_rect.size(), items_pixmap );
	    highlight_pixmap = style->hilite.render( Size( itemw, itemh ),
						     highlight_pixmap );
	}
    }

    indent = itemh;

    Size sz( w, h );
    if ( sz != size() )
	resize( sz );
    size_dirty = false;
}

void Basemenu2::popup( int x, int y, bool centerOnTitle )
{
    motion = 0;
    if ( size_dirty )
	updateSize();

    if ( show_title ) {
	if ( centerOnTitle ) {
	    // if the title is visible, center it around the popup point
	    move( Point( x, y ) - title_rect.pos() -
		  Point( title_rect.width() / 2, title_rect.height() / 2 ) );
	} else {
	    move( x, y - title_rect.y() );
	}
    } else
	move( x, y );

    show();
}

void Basemenu2::popup( const Point &p, bool centerOnTitle )
{
    popup( p.x(), p.y(), centerOnTitle );
}

void Basemenu2::hide()
{
    if ( current_submenu )
	current_submenu->hide();
    current_submenu = 0;

    slist<Item>::iterator it = items.begin();
    if ( show_title ) {
	if ( it == items.end() ){
	    // internal error
	    fprintf( stderr, "Basemenu2: cannot track active item, internal error\n" );
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

void Basemenu2::hideAll()
{
    if ( parent_menu )
	parent_menu->hide();
    else
	hide();
}

int Basemenu2::insert( const string &label, const Item &item, int index )
{
    slist<Item>::iterator it;

    if ( index >= 0 ) {
	it = items.begin();

	if ( show_title ) {
	    if ( it == items.end() ) {
		// internal error
		fprintf( stderr, "Basemenu2: cannot insert item, internal error\n" );
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

void Basemenu2::change( int index,
			const string &label,
			const Item &item = Item::Default )
{
    slist<Item>::iterator it = items.begin();

    if ( show_title ) {
	if ( it == items.end() ) {
	    // internal error
	    fprintf( stderr, "Basemenu2: cannot change item, internal error\n" );
	    return;
	}

	it++;
    }

    int i = 0;
    while ( i++ < index ) {
	if ( it == items.end() ) {
	    // index is out of range
	    fprintf( stderr, "Basemenu2: cannot change item %d, index out of range\n",
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

void Basemenu2::remove( int index )
{
    slist<Item>::iterator it = items.begin();

    if ( show_title ) {
	if ( it == items.end() ) {
	    // index is out of range
	    fprintf( stderr, "Basemenu2: cannot remove item, internal error\n" );
	    return;
	}

	it++;
    }

    int i = 0;
    while ( i++ < index ) {
	if ( it == items.end() ) {
	    // index is out of range
	    fprintf( stderr, "Basemenu2: cannot remove item %d, index out of range\n",
		     index );
	    return;
	}

	it++;
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

bool Basemenu2::isItemEnabled( int index ) const
{
    slist<Item>::const_iterator it = items.begin();

    if ( show_title ) {
	if ( it == items.end() ) {
	    // internal error
	    fprintf( stderr, "Basemenu2: cannot find item, internal error\n" );
	    return false;
	}

	it++;
    }

    int i = 0;
    while ( i++ < index ) {
	if ( it == items.end() ) {
	    // index is out of range
	    fprintf( stderr, "Basemenu2: cannot find item %d, index out of range\n",
		     index );
	    return false;
	}

	it++;
    }

    return (*it).isEnabled();
}

void Basemenu2::setItemEnabled( int index, bool enabled )
{
    slist<Item>::iterator it = items.begin();

    if ( show_title ) {
	if ( it == items.end() ) {
	    // internal error
	    fprintf( stderr, "Basemenu2: cannot enable item, internal error\n" );
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
	    fprintf( stderr, "Basemenu2: cannot enable item %d, index out of range\n",
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

    (*it).enable = enabled;
    if ( isVisible() )
	XClearArea( *BaseDisplay::instance(), windowID(),
		    r.x(), r.y(), r.width(), r.height(), True );
}

bool Basemenu2::isItemChecked( int index ) const
{
    slist<Item>::const_iterator it = items.begin();

    if ( show_title ) {
	if ( it == items.end() ) {
	    // internal error
	    fprintf( stderr, "Basemenu2: cannot find item, internal error\n" );
	    return false;
	}

	it++;
    }

    int i = 0;
    while ( i++ < index ) {
	if ( it == items.end() ) {
	    // index is out of range
	    fprintf( stderr, "Basemenu2: cannot find item %d, index out of range\n",
		     index );
	    return false;
	}

	it++;
    }

    return (*it).isChecked();
}

void Basemenu2::setItemChecked( int index, bool check )
{
    slist<Item>::iterator it = items.begin();

    if ( show_title ) {
	if ( it == items.end() ) {
	    // internal error
	    fprintf( stderr, "Basemenu2: cannot enable item, internal error\n" );
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
	    fprintf( stderr, "Basemenu2: cannot enable item %d, index out of range\n",
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

    (*it).checked = check;
    if ( isVisible() )
	XClearArea( *BaseDisplay::instance(), windowID(),
		    r.x(), r.y(), r.width(), r.height(), True );
}

void Basemenu2::showTitle()
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

void Basemenu2::hideTitle()
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

void Basemenu2::setActiveItem( const Rect &r, Item &item )
{
    if ( item.active )
	return;

    item.active = item.isEnabled();
    drawItem( r, item );
    showSubmenu( r, item );
}

void Basemenu2::showSubmenu( const Rect &r, const Item &item )
{
    if ( current_submenu )
	current_submenu->hide();
    current_submenu = 0;

    if ( ! item.submenu() )
	return;

    // show the submenu
    Point p = pos() + Point( r.x() + r.width(), r.y() - 1 );
    current_submenu = item.submenu();
    current_submenu->popup( p, false );
}

void Basemenu2::buttonPressEvent( XEvent *e )
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

    slist<Item>::iterator it = items.begin();
    if ( show_title ) {
	if ( it == items.end() ){
	    // internal error
	    fprintf( stderr, "Basemenu2: cannot track active item, internal error\n" );
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

void Basemenu2::buttonReleaseEvent( XEvent *e )
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

    slist<Item>::iterator it = items.begin();
    if ( show_title ) {
	if ( it == items.end() ){
	    // internal error
	    fprintf( stderr, "Basemenu2: cannot track active item, internal error\n" );
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
	    if ( ! item.submenu() ) {
		if ( item.isEnabled() )
		    itemClicked( p - items_rect.pos(), item, e->xbutton.button );
	    } else
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

void Basemenu2::pointerMotionEvent( XEvent *e )
{
    motion++;

    Point p( e->xmotion.x, e->xmotion.y );
    if ( ! items_rect.contains( p ) && ( show_title && ! title_rect.contains( p ) ) )
	return;

    slist<Item>::iterator it = items.begin();
    if ( show_title ) {
	if ( it == items.end() ){
	    // internal error
	    fprintf( stderr, "Basemenu2: cannot track active item, internal error\n" );
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

void Basemenu2::enterEvent( XEvent * )
{
}

void Basemenu2::leaveEvent( XEvent * )
{
    slist<Item>::iterator it = items.begin();
    if ( show_title ) {
	if ( it == items.end() ){
	    // internal error
	    fprintf( stderr, "Basemenu2: cannot track active item, internal error\n" );
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

	if ( item.active && item.submenu() != current_submenu ) {
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

void Basemenu2::exposeEvent( XEvent *e )
{
    Rect todo( e->xexpose.x, e->xexpose.y, e->xexpose.width, e->xexpose.height );
    MenuStyle *style = Blackbox::instance()->screen( screen() )->getMenuStyle();
    BaseDisplay *display = BaseDisplay::instance();
    BGCCache *cache = BGCCache::instance();
    BGCCache::Item &tgc = cache->find( style->title.color() ),
		   &igc = cache->find( style->frame.color() );
    if ( show_title && todo.intersects( title_rect ) ) {
	Rect up = title_rect & todo;
	if ( style->title.texture() == ( BImage_Solid | BImage_Flat ) )
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
	if ( style->frame.texture() == ( BImage_Solid | BImage_Flat ) )
	    XFillRectangle( *BaseDisplay::instance(), windowID(), igc.gc(),
			    up.x(), up.y(), up.width(), up.height() );
	else if ( items_pixmap )
	    XCopyArea( *display, items_pixmap, windowID(), igc.gc(),
		       up.x() - items_rect.x(), up.y() - items_rect.y(),
		       up.width(), up.height(), up.x(), up.y() );


	slist<Item>::const_iterator it = items.begin();
	if ( show_title ) {
	    if ( it == items.end() ) {
		// internal error
		fprintf( stderr, "Basemenu2: cannot draw menu items, internal error\n" );
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

void Basemenu2::mapEvent( XEvent *e )
{
    Widget::mapEvent( e );
}

void Basemenu2::unmapEvent( XEvent *e )
{
    Widget::unmapEvent( e );
}

void Basemenu2::configureEvent( XEvent *e )
{
    Widget::configureEvent( e );
}

void Basemenu2::titleClicked( const Point &, int button )
{
    if ( button == 3 )
	hideAll();
}

void Basemenu2::itemClicked( const Point &, const Item &, int )
{
}


























// old basemenu

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#include <algorithm>
using namespace std;

#include "i18n.hh"
#include "blackbox.hh"
#include "Basemenu.hh"
#include "GCCache.hh"
#include "Screen.hh"

static Basemenu *shown = (Basemenu *) 0;


Basemenu::Basemenu(BScreen *scrn) {
  screen = scrn;
  blackbox = screen->getBlackbox();
  image_ctrl = screen->getImageControl();
  parent = (Basemenu *) 0;
  alignment = AlignDontCare;

  title_vis =
    movable =
    hide_tree = True;

  shifted =
    internal_menu =
    moving =
    torn =
    visible = False;

  menu.x =
    menu.y =
    menu.x_shift =
    menu.y_shift =
    menu.x_move =
    menu.y_move = 0;

  which_sub =
    which_press =
    which_sbl = -1;

  menu.frame_pixmap =
    menu.title_pixmap =
    menu.hilite_pixmap =
    menu.sel_pixmap = None;

  menu.bevel_w = screen->getBevelWidth();

  if (i18n->multibyte())
    menu.width = menu.title_h = menu.item_w = menu.frame_h =
      screen->getMenuStyle()->t_fontset_extents->max_ink_extent.height +
	(menu.bevel_w  * 2);
  else
    menu.width = menu.title_h = menu.item_w = menu.frame_h =
      screen->getMenuStyle()->t_font->ascent +
        screen->getMenuStyle()->t_font->descent + (menu.bevel_w * 2);

  menu.label = 0;

  menu.sublevels =
    menu.persub =
    menu.minsub = 0;

  MenuStyle *style = screen->getMenuStyle();
  if (i18n->multibyte()) {
    menu.item_h = style->f_fontset_extents->max_ink_extent.height +
      (menu.bevel_w);
  } else {
    menu.item_h = style->f_font->ascent + style->f_font->descent +
		  (menu.bevel_w);
  }

  menu.height = menu.title_h + screen->getBorderWidth() + menu.frame_h;

  unsigned long attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
			      CWColormap | CWOverrideRedirect | CWEventMask;
  XSetWindowAttributes attrib;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel =
			    screen->getBorderColor()->pixel();
  attrib.colormap = screen->colormap();
  attrib.override_redirect = True;
  attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
                      ButtonMotionMask | ExposureMask;

  menu.window =
    XCreateWindow(*BaseDisplay::instance(), screen->rootWindow(), menu.x, menu.y, menu.width,
		  menu.height, screen->getBorderWidth(), screen->depth(),
                  InputOutput, screen->visual(), attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.window, this);

  attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel | CWEventMask;
  attrib.background_pixel = screen->getBorderColor()->pixel();
  attrib.event_mask |= EnterWindowMask | LeaveWindowMask;

  menu.title =
    XCreateWindow(*BaseDisplay::instance(), menu.window, 0, 0, menu.width, menu.height, 0,
		  screen->depth(), InputOutput, screen->visual(),
		  attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.title, this);

  attrib.event_mask |= PointerMotionMask;
  menu.frame = XCreateWindow(*BaseDisplay::instance(), menu.window, 0,
			     menu.title_h + screen->getBorderWidth(),
			     menu.width, menu.frame_h, 0,
			     screen->depth(), InputOutput,
			     screen->visual(), attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.frame, this);

  menuitems = new LinkedList<BasemenuItem>;

  // even though this is the end of the constructor the menu is still not
  // completely created.  items must be inserted and it must be update()'d
}


Basemenu::~Basemenu(void) {
  XUnmapWindow(*BaseDisplay::instance(), menu.window);

  if (shown && shown->getWindowID() == getWindowID())
    shown = (Basemenu *) 0;

  int n = menuitems->count();
  for (int i = 0; i < n; ++i)
    remove(0);

  delete menuitems;

  if (menu.label)
    delete [] menu.label;

  if (menu.title_pixmap)
    image_ctrl->removeImage(menu.title_pixmap);

  if (menu.frame_pixmap)
    image_ctrl->removeImage(menu.frame_pixmap);

  if (menu.hilite_pixmap)
    image_ctrl->removeImage(menu.hilite_pixmap);

  if (menu.sel_pixmap)
    image_ctrl->removeImage(menu.sel_pixmap);

  blackbox->removeMenuSearch(menu.title);
  XDestroyWindow(*BaseDisplay::instance(), menu.title);

  blackbox->removeMenuSearch(menu.frame);
  XDestroyWindow(*BaseDisplay::instance(), menu.frame);

  blackbox->removeMenuSearch(menu.window);
  XDestroyWindow(*BaseDisplay::instance(), menu.window);
}


int Basemenu::insert(const char *l, int function, const char *e, int pos) {
  char *label = 0, *exec = 0;

  if (l) label = bstrdup(l);
  if (e) exec = bstrdup(e);

  BasemenuItem *item = new BasemenuItem(label, function, exec);
  menuitems->insert(item, pos);

  return menuitems->count();
}


int Basemenu::insert(const char *l, Basemenu *submenu, int pos) {
  char *label = 0;

  if (l) label = bstrdup(l);

  BasemenuItem *item = new BasemenuItem(label, submenu);
  menuitems->insert(item, pos);

  submenu->parent = this;

  return menuitems->count();
}


int Basemenu::insert(const char **ulabel, int pos, int function) {
  BasemenuItem *item = new BasemenuItem(ulabel, function);
  menuitems->insert(item, pos);

  return menuitems->count();
}


int Basemenu::remove(int index) {
  if (index < 0 || index > menuitems->count()) return -1;

  BasemenuItem *item = menuitems->remove(index);

  if (item) {
    if ((! internal_menu) && (item->submenu())) {
      Basemenu *tmp = (Basemenu *) item->submenu();

      if (! tmp->internal_menu) {
	delete tmp;
      } else {
	tmp->internal_hide();
      }
    }

    if (item->label())
      delete [] item->label();

    if (item->exec())
      delete [] item->exec();

    delete item;
  }

  if (which_sub == index)
    which_sub = -1;
  else if (which_sub > index)
    which_sub--;

  return menuitems->count();
}


void Basemenu::update(void) {
  MenuStyle *style = screen->getMenuStyle();
  if (i18n->multibyte()) {
    menu.item_h = style->f_fontset_extents->max_ink_extent.height +
		  menu.bevel_w;
    menu.title_h = style->t_fontset_extents->max_ink_extent.height +
                   (menu.bevel_w * 2);
  } else {
    menu.item_h = style->f_font->ascent + style->f_font->descent +
		  menu.bevel_w;
    menu.title_h = style->t_font->ascent + style->t_font->descent +
		   (menu.bevel_w * 2);
  }

  if (title_vis) {
    const char *s = (menu.label) ? menu.label :
		    i18n->getMessage(BasemenuSet, BasemenuBlackboxMenu,
				     "Blackbox Menu");
    int l = strlen(s);


    if (i18n->multibyte()) {
      XRectangle ink, logical;
      XmbTextExtents(screen->getMenuStyle()->t_fontset, s, l, &ink, &logical);
      menu.item_w = logical.width;
    } else {
      menu.item_w = XTextWidth(screen->getMenuStyle()->t_font, s, l);
    }

    menu.item_w += (menu.bevel_w * 2);
  }  else {
    menu.item_w = 1;
  }

  int ii = 0;
  LinkedListIterator<BasemenuItem> it(menuitems);
  for (BasemenuItem *tmp = it.current(); tmp; it++, tmp = it.current()) {
    const char *s = ((tmp->u && *tmp->u) ? *tmp->u :
		     ((tmp->l) ? tmp->l : (const char *) 0));
    int l = strlen(s);

    if (i18n->multibyte()) {
      XRectangle ink, logical;
      XmbTextExtents(screen->getMenuStyle()->f_fontset, s, l, &ink, &logical);
      ii = logical.width;
    } else
      ii = XTextWidth(screen->getMenuStyle()->f_font, s, l);

    ii += (menu.bevel_w * 2) + (menu.item_h * 2);

    menu.item_w = ((menu.item_w < (unsigned int) ii) ? ii : menu.item_w);
  }

  if (menuitems->count()) {
    menu.sublevels = 1;

    while (((menu.item_h * (menuitems->count() + 1) / menu.sublevels)
	    + menu.title_h + screen->getBorderWidth()) >
	   screen->height())
      menu.sublevels++;

    if (menu.sublevels < menu.minsub) menu.sublevels = menu.minsub;

    menu.persub = menuitems->count() / menu.sublevels;
    if (menuitems->count() % menu.sublevels) menu.persub++;
  } else {
    menu.sublevels = 0;
    menu.persub = 0;
  }

  menu.width = (menu.sublevels * (menu.item_w));
  if (! menu.width) menu.width = menu.item_w;

  menu.frame_h = (menu.item_h * menu.persub);
  menu.height = ((title_vis) ? menu.title_h + screen->getBorderWidth() : 0) +
		menu.frame_h;
  if (! menu.frame_h) menu.frame_h = 1;
  if (menu.height < 1) menu.height = 1;

  Pixmap tmp;
  BTexture texture;
  if (title_vis) {
    tmp = menu.title_pixmap;
    texture = screen->getMenuStyle()->title;
    if (texture.texture() == (BImage_Flat | BImage_Solid)) {
      menu.title_pixmap = None;
      XSetWindowBackground(*BaseDisplay::instance(), menu.title,
			   texture.color().pixel());
    } else {
      menu.title_pixmap =
        image_ctrl->renderImage(menu.width, menu.title_h, texture);
      XSetWindowBackgroundPixmap(*BaseDisplay::instance(), menu.title, menu.title_pixmap);
    }
    if (tmp) image_ctrl->removeImage(tmp);
    XClearWindow(*BaseDisplay::instance(), menu.title);
  }

  tmp = menu.frame_pixmap;
  texture = screen->getMenuStyle()->frame;
  if (texture.texture() == (BImage_Flat | BImage_Solid)) {
    menu.frame_pixmap = None;
    XSetWindowBackground(*BaseDisplay::instance(), menu.frame,
			 texture.color().pixel());
  } else {
    menu.frame_pixmap =
      image_ctrl->renderImage(menu.width, menu.frame_h, texture);
    XSetWindowBackgroundPixmap(*BaseDisplay::instance(), menu.frame, menu.frame_pixmap);
  }
  if (tmp) image_ctrl->removeImage(tmp);

  tmp = menu.hilite_pixmap;
  texture = screen->getMenuStyle()->hilite;
  if (texture.texture() == (BImage_Flat | BImage_Solid)) {
    menu.hilite_pixmap = None;
  } else {
    menu.hilite_pixmap =
      image_ctrl->renderImage(menu.item_w, menu.item_h, texture);
  }
  if (tmp) image_ctrl->removeImage(tmp);

  tmp = menu.sel_pixmap;
  if (texture.texture() == (BImage_Flat | BImage_Solid)) {
    menu.sel_pixmap = None;
  } else {
    int hw = menu.item_h / 2;
    menu.sel_pixmap =
      image_ctrl->renderImage(hw, hw, texture);
  }
  if (tmp) image_ctrl->removeImage(tmp);

  XResizeWindow(*BaseDisplay::instance(), menu.window, menu.width, menu.height);

  if (title_vis)
    XResizeWindow(*BaseDisplay::instance(), menu.title, menu.width, menu.title_h);

  XMoveResizeWindow(*BaseDisplay::instance(), menu.frame, 0,
		    ((title_vis) ? menu.title_h +
		     screen->getBorderWidth() : 0), menu.width,
		    menu.frame_h);

  XClearWindow(*BaseDisplay::instance(), menu.window);
  XClearWindow(*BaseDisplay::instance(), menu.title);
  XClearWindow(*BaseDisplay::instance(), menu.frame);

  if (title_vis && visible) redrawTitle();

  for (int i = 0; visible && i < menuitems->count(); i++) {
    if (i == which_sub) {
      drawItem(i, True, 0);
      drawSubmenu(i);
    } else {
      drawItem(i, False, 0);
    }
  }

  if (parent && visible)
    parent->drawSubmenu(parent->which_sub);

  XMapSubwindows(*BaseDisplay::instance(), menu.window);
}


void Basemenu::show(void) {
  XMapSubwindows(*BaseDisplay::instance(), menu.window);
  XMapWindow(*BaseDisplay::instance(), menu.window);
  visible = True;

  if (! parent) {
    if (shown && (! shown->torn))
       shown->hide();

    shown = this;
  }
}


void Basemenu::hide(void) {
  if ((! torn) && hide_tree && parent && parent->isVisible()) {
    Basemenu *p = parent;

    while (p->isVisible() && (! p->torn) && p->parent) p = p->parent;
    p->internal_hide();
  } else {
    internal_hide();
  }
}


void Basemenu::internal_hide(void) {
  if (which_sub != -1) {
    BasemenuItem *tmp = menuitems->find(which_sub);
    tmp->submenu()->internal_hide();
  }

  if (parent && (! torn)) {
    parent->drawItem(parent->which_sub, False, True);

    parent->which_sub = -1;
  } else if (shown && shown->menu.window == menu.window) {
    shown = (Basemenu *) 0;
  }

  torn = visible = False;
  which_sub = which_press = which_sub = -1;

  XUnmapWindow(*BaseDisplay::instance(), menu.window);
}


void Basemenu::move(int x, int y) {
  menu.x = x;
  menu.y = y;
  XMoveWindow(*BaseDisplay::instance(), menu.window, x, y);
  if (which_sub != -1)
    drawSubmenu(which_sub);
}


void Basemenu::redrawTitle(void) {
    char *text = (char *) ((menu.label) ? menu.label :
			   i18n->getMessage(BasemenuSet, BasemenuBlackboxMenu,
					    "Blackbox Menu"));
    int dx = menu.bevel_w, len = strlen(text);
    unsigned int l;

    if (i18n->multibyte()) {
	XRectangle ink, logical;
	XmbTextExtents(screen->getMenuStyle()->t_fontset, text, len, &ink, &logical);
	l = logical.width;
    } else {
	l = XTextWidth(screen->getMenuStyle()->t_font, text, len);
    }

    l +=  (menu.bevel_w * 2);

    switch (screen->getMenuStyle()->t_justify) {
    case BScreen::RightJustify:
	dx += menu.width - l;
	break;

    case BScreen::CenterJustify:
	dx += (menu.width - l) / 2;
	break;
    }

    MenuStyle *style = screen->getMenuStyle();
    BGCCache::Item &gc = BGCCache::instance()->find( style->t_text, style->t_font );
    if (i18n->multibyte()) {
	XmbDrawString(*BaseDisplay::instance(), menu.title, style->t_fontset, gc.gc(),
		      dx, (menu.bevel_w - style->t_fontset_extents->max_ink_extent.y),
		      text, len);
    } else {
	XDrawString(*BaseDisplay::instance(), menu.title, gc.gc(),
		    dx, (style->t_font->ascent + menu.bevel_w), text, len);
    }
    BGCCache::instance()->release( gc );
}


void Basemenu::drawSubmenu(int index) {
  if (which_sub != -1 && which_sub != index) {
    BasemenuItem *itmp = menuitems->find(which_sub);

    if (! itmp->submenu()->isTorn())
      itmp->submenu()->internal_hide();
  }

  if (index >= 0 && index < menuitems->count()) {
    BasemenuItem *item = menuitems->find(index);
    if (item->submenu() && visible && (! item->submenu()->isTorn()) &&
	item->isEnabled()) {
      if (item->submenu()->parent != this) item->submenu()->parent = this;
      int sbl = index / menu.persub, i = index - (sbl * menu.persub),
	    x = menu.x +
		((menu.item_w * (sbl + 1)) + screen->getBorderWidth()), y;

      if (alignment == AlignTop)
	y = (((shifted) ? menu.y_shift : menu.y) +
	     ((title_vis) ? menu.title_h + screen->getBorderWidth() : 0) -
	     ((item->submenu()->title_vis) ?
	      item->submenu()->menu.title_h + screen->getBorderWidth() : 0));
      else
	y = (((shifted) ? menu.y_shift : menu.y) +
	     (menu.item_h * i) +
	     ((title_vis) ? menu.title_h + screen->getBorderWidth() : 0) -
	     ((item->submenu()->title_vis) ?
	      item->submenu()->menu.title_h + screen->getBorderWidth() : 0));

      if (alignment == AlignBottom &&
	  (y + item->submenu()->menu.height) > ((shifted) ? menu.y_shift :
						menu.y) + menu.height)
	y = (((shifted) ? menu.y_shift : menu.y) +
	     menu.height - item->submenu()->menu.height);

      if ((x + item->submenu()->width()) > screen->width()) {
	x = ((shifted) ? menu.x_shift : menu.x) -
	    item->submenu()->width() - screen->getBorderWidth();
      }

      if (x < 0) x = 0;

      if ((y + item->submenu()->height()) > screen->height())
	y = screen->height() - item->submenu()->height() -
	    (screen->getBorderWidth() * 2);
      if (y < 0) y = 0;

      item->submenu()->move(x, y);
      if (! moving) drawItem(index, True);

      if (! item->submenu()->isVisible())
	item->submenu()->show();
      item->submenu()->moving = moving;
      which_sub = index;
    } else {
      which_sub = -1;
    }
  }
}


Bool Basemenu::hasSubmenu(int index) {
  if ((index >= 0) && (index < menuitems->count()))
    if (menuitems->find(index)->submenu())
      return True;

  return False;
}


void Basemenu::drawItem(int index, Bool highlight, Bool clear,
			int x, int y, unsigned int w, unsigned int h)
{
  if (index < 0 || index > menuitems->count()) return;

  BasemenuItem *item = menuitems->find(index);
  if (! item) return;

  Bool dotext = True, dohilite = True, dosel = True;
  const char *text = (item->ulabel()) ? *item->ulabel() : item->label();
  int sbl = index / menu.persub, i = index - (sbl * menu.persub);
  int item_x = (sbl * menu.item_w), item_y = (i * menu.item_h);
  int hilite_x = item_x, hilite_y = item_y, hoff_x = 0, hoff_y = 0;
  int text_x = 0, text_y = 0, len = strlen(text), sel_x = 0, sel_y = 0;
  unsigned int hilite_w = menu.item_w, hilite_h = menu.item_h, text_w = 0,
    text_h = 0;
  unsigned int half_w = menu.item_h / 2, quarter_w = menu.item_h / 4;

  if (text) {
    if (i18n->multibyte()) {
      XRectangle ink, logical;
      XmbTextExtents(screen->getMenuStyle()->f_fontset,
		     text, len, &ink, &logical);
      text_w = logical.width;
      text_y = item_y + (menu.bevel_w / 2) -
	       screen->getMenuStyle()->f_fontset_extents->max_ink_extent.y;
    } else {
      text_w = XTextWidth(screen->getMenuStyle()->f_font, text, len);
      text_y =  item_y +
		screen->getMenuStyle()->f_font->ascent +
		(menu.bevel_w / 2);
    }

    switch(screen->getMenuStyle()->f_justify) {
    case BScreen::LeftJustify:
      text_x = item_x + menu.bevel_w + menu.item_h + 1;
      break;

    case BScreen::RightJustify:
      text_x = item_x + menu.item_w - (menu.item_h + menu.bevel_w + text_w);
      break;

    case BScreen::CenterJustify:
      text_x = item_x + ((menu.item_w + 1 - text_w) / 2);
      break;
    }

    text_h = menu.item_h - menu.bevel_w;
  }

  MenuStyle *style = screen->getMenuStyle();
  BGCCache *cache = BGCCache::instance();
  BGCCache::Item &gc = cache->find( ( ( highlight || item->isSelected() ) ?
				      style->h_text : style->f_text ), style->f_font ),
		&tgc = cache->find( (highlight) ? screen->getMenuStyle()->h_text :
				    ( ( item->isEnabled() ) ?
				      screen->getMenuStyle()->f_text :
				      screen->getMenuStyle()->d_text ), style->f_font ),
		&hgc = cache->find( screen->getMenuStyle()->hilite.color(),
				    style->f_font );

  sel_x = item_x;
  if (screen->getMenuStyle()->bullet_pos == Right)
    sel_x += (menu.item_w - menu.item_h - menu.bevel_w);
  sel_x += quarter_w;
  sel_y = item_y + quarter_w;

  if (clear) {
    XClearArea(*BaseDisplay::instance(), menu.frame, item_x, item_y, menu.item_w, menu.item_h,
	       False);
  } else if (! (x == y && y == -1 && w == h && h == 0)) {
    // calculate the which part of the hilite to redraw
    if (! (max(item_x, x) <= (signed) min(item_x + menu.item_w, x + w) &&
	   max(item_y, y) <= (signed) min(item_y + menu.item_h, y + h))) {
      dohilite = False;
    } else {
      hilite_x = max(item_x, x);
      hilite_y = max(item_y, y);
      hilite_w = min(item_x + menu.item_w, x + w) - hilite_x;
      hilite_h = min(item_y + menu.item_h, y + h) - hilite_y;
      hoff_x = hilite_x % menu.item_w;
      hoff_y = hilite_y % menu.item_h;
    }

    // check if we need to redraw the text
    int text_ry = item_y + (menu.bevel_w / 2);
    if (! (max(text_x, x) <= (signed) min(text_x + text_w, x + w) &&
	   max(text_ry, y) <= (signed) min(text_ry + text_h, y + h)))
      dotext = False;

    // check if we need to redraw the select pixmap/menu bullet
    if (! (max(sel_x, x) <= (signed) min(sel_x + half_w, x + w) &&
	   max(sel_y, y) <= (signed) min(sel_y + half_w, y + h)))
      dosel = False;
  }

  if (dohilite && highlight && (menu.hilite_pixmap != ParentRelative)) {
    if (menu.hilite_pixmap)
      XCopyArea(*BaseDisplay::instance(), menu.hilite_pixmap, menu.frame, hgc.gc(),
		hoff_x, hoff_y, hilite_w, hilite_h, hilite_x, hilite_y);
    else
      XFillRectangle(*BaseDisplay::instance(), menu.frame, hgc.gc(),
		     hilite_x, hilite_y, hilite_w, hilite_h);
  } else if (dosel && item->isSelected() &&
	                 (menu.sel_pixmap != ParentRelative)) {
    if (menu.sel_pixmap)
      XCopyArea(*BaseDisplay::instance(), menu.sel_pixmap, menu.frame, hgc.gc(),
		0, 0, half_w, half_w, sel_x, sel_y);
    else
      XFillRectangle(*BaseDisplay::instance(), menu.frame, hgc.gc(), sel_x, sel_y, half_w, half_w);
  }

  if (dotext && text) {
    if (i18n->multibyte())
      XmbDrawString(*BaseDisplay::instance(), menu.frame, screen->getMenuStyle()->f_fontset,
		    tgc.gc(), text_x, text_y, text, len);
    else
      XDrawString(*BaseDisplay::instance(), menu.frame, tgc.gc(), text_x, text_y, text, len);
  }

  if (dosel && item->submenu()) {
    switch (screen->getMenuStyle()->bullet) {
    case Square:
      XDrawRectangle(*BaseDisplay::instance(), menu.frame, gc.gc(), sel_x, sel_y, half_w, half_w);
      break;

    case Triangle:
      XPoint tri[3];

      if (screen->getMenuStyle()->bullet_pos == Right) {
        tri[0].x = sel_x + quarter_w - 2;
	tri[0].y = sel_y + quarter_w - 2;
        tri[1].x = 4;
	tri[1].y = 2;
        tri[2].x = -4;
	tri[2].y = 2;
      } else {
        tri[0].x = sel_x + quarter_w - 2;
	tri[0].y = item_y + half_w;
        tri[1].x = 4;
	tri[1].y = 2;
        tri[2].x = 0;
	tri[2].y = -4;
      }

      XFillPolygon(*BaseDisplay::instance(), menu.frame, gc.gc(), tri, 3, Convex,
                   CoordModePrevious);
      break;

    case Diamond:
      XPoint dia[4];

      dia[0].x = sel_x + quarter_w - 3;
      dia[0].y = item_y + half_w;
      dia[1].x = 3;
      dia[1].y = -3;
      dia[2].x = 3;
      dia[2].y = 3;
      dia[3].x = -3;
      dia[3].y = 3;

      XFillPolygon(*BaseDisplay::instance(), menu.frame, gc.gc(), dia, 4, Convex,
                   CoordModePrevious);
      break;
    }
  }

  cache->release( gc );
  cache->release( tgc );
  cache->release( hgc );
}


void Basemenu::setLabel(const char *l) {
  if (menu.label)
    delete [] menu.label;

  if (l) menu.label = bstrdup(l);
  else menu.label = 0;
}


void Basemenu::setItemSelected(int index, Bool sel) {
  if (index < 0 || index >= menuitems->count()) return;

  BasemenuItem *item = find(index);
  if (! item) return;

  item->setSelected(sel);
  if (visible) drawItem(index, (index == which_sub), True);
}


Bool Basemenu::isItemSelected(int index) {
  if (index < 0 || index >= menuitems->count()) return False;

  BasemenuItem *item = find(index);
  if (! item) return False;

  return item->isSelected();
}


void Basemenu::setItemEnabled(int index, Bool enable) {
  if (index < 0 || index >= menuitems->count()) return;

  BasemenuItem *item = find(index);
  if (! item) return;

  item->setEnabled(enable);
  if (visible) drawItem(index, (index == which_sub), True);
}


Bool Basemenu::isItemEnabled(int index) {
  if (index < 0 || index >= menuitems->count()) return False;

  BasemenuItem *item = find(index);
  if (! item) return False;

  return item->isEnabled();
}


void Basemenu::buttonPressEvent(XButtonEvent *be) {
  if (be->window == menu.frame) {
    int sbl = (be->x / menu.item_w), i = (be->y / menu.item_h);
    int w = (sbl * menu.persub) + i;

    if (w < menuitems->count() && w >= 0) {
      which_press = i;
      which_sbl = sbl;

      BasemenuItem *item = menuitems->find(w);

      if (item->submenu())
	drawSubmenu(w);
      else
	drawItem(w, (item->isEnabled()), True);
    }
  } else {
    menu.x_move = be->x_root - menu.x;
    menu.y_move = be->y_root - menu.y;
  }
}


void Basemenu::buttonReleaseEvent(XButtonEvent *re) {
  if (re->window == menu.title) {
    if (moving) {
      moving = False;

      if (which_sub != -1)
	drawSubmenu(which_sub);
    }

    if (re->x >= 0 && re->x <= (signed) menu.width &&
	re->y >= 0 && re->y <= (signed) menu.title_h)
      if (re->button == 3)
	hide();
  } else if (re->window == menu.frame &&
	     re->x >= 0 && re->x < (signed) menu.width &&
	     re->y >= 0 && re->y < (signed) menu.frame_h) {
    if (re->button == 3) {
      hide();
    } else {
      int sbl = (re->x / menu.item_w), i = (re->y / menu.item_h),
	   ix = sbl * menu.item_w, iy = i * menu.item_h,
	    w = (sbl * menu.persub) + i,
	    p = (which_sbl * menu.persub) + which_press;

      if (w < menuitems->count() && w >= 0) {
	drawItem(p, (p == which_sub), True);

        if  (p == w && isItemEnabled(w)) {
	  if (re->x > ix && re->x < (signed) (ix + menu.item_w) &&
	      re->y > iy && re->y < (signed) (iy + menu.item_h)) {
	    itemSelected(re->button, w);
	  }
        }
      } else
        drawItem(p, False, True);
    }
  }
}


void Basemenu::motionNotifyEvent(XMotionEvent *me) {
  if (me->window == menu.title && (me->state & Button1Mask)) {
    if (movable) {
      if (! moving) {
	if (parent && (! torn)) {
	  parent->drawItem(parent->which_sub, False, True);
	  parent->which_sub = -1;
	}

        moving = torn = True;

	if (which_sub != -1)
	  drawSubmenu(which_sub);
      } else {
	menu.x = me->x_root - menu.x_move,
	menu.y = me->y_root - menu.y_move;

	XMoveWindow(*BaseDisplay::instance(), menu.window, menu.x, menu.y);

	if (which_sub != -1)
	  drawSubmenu(which_sub);
      }
    }
  } else if ((! (me->state & Button1Mask)) && me->window == menu.frame &&
	     me->x >= 0 && me->x < (signed) menu.width &&
	     me->y >= 0 && me->y < (signed) menu.frame_h) {
    int sbl = (me->x / menu.item_w), i = (me->y / menu.item_h),
	  w = (sbl * menu.persub) + i;

    if ((i != which_press || sbl != which_sbl) &&
	(w < menuitems->count() && w >= 0)) {
      if (which_press != -1 && which_sbl != -1) {
	int p = (which_sbl * menu.persub) + which_press;
	BasemenuItem *item = menuitems->find(p);

	drawItem(p, False, True);
	if (item->submenu())
	  if (item->submenu()->isVisible() &&
	      (! item->submenu()->isTorn())) {
	    item->submenu()->internal_hide();
	    which_sub = -1;
	  }
      }

      which_press = i;
      which_sbl = sbl;

      BasemenuItem *itmp = menuitems->find(w);

      if (itmp->submenu())
	drawSubmenu(w);
      else
	drawItem(w, (itmp->isEnabled()), True);
    }
  }
}


void Basemenu::exposeEvent(XExposeEvent *ee) {
  if (ee->window == menu.title) {
    redrawTitle();
  } else if (ee->window == menu.frame) {
    // this is a compilicated algorithm... lets do it step by step...
    // first... we see in which sub level the expose starts... and how many
    // items down in that sublevel

    int sbl = (ee->x / menu.item_w), id = (ee->y / menu.item_h),
      // next... figure out how many sublevels over the redraw spans
      sbl_d = ((ee->x + ee->width) / menu.item_w),
      // then we see how many items down to redraw
      id_d = ((ee->y + ee->height) / menu.item_h);

    if (id_d > menu.persub) id_d = menu.persub;

    // draw the sublevels and the number of items the exposure spans
    LinkedListIterator<BasemenuItem> it(menuitems);
    int i, ii;
    for (i = sbl; i <= sbl_d; i++) {
      // set the iterator to the first item in the sublevel needing redrawing
      it.set(id + (i * menu.persub));
      for (ii = id; ii <= id_d && it.current(); it++, ii++) {
	int index = ii + (i * menu.persub);
	// redraw the item
	drawItem(index, (which_sub == index), False,
		 ee->x, ee->y, ee->width, ee->height);
      }
    }
  }
}


void Basemenu::enterNotifyEvent(XCrossingEvent *ce) {
  if (ce->window == menu.frame) {
    menu.x_shift = menu.x, menu.y_shift = menu.y;
    if (menu.x + menu.width > screen->width()) {
      menu.x_shift = screen->width() - menu.width -
        screen->getBorderWidth();
      shifted = True;
    } else if (menu.x < 0) {
      menu.x_shift = -screen->getBorderWidth();
      shifted = True;
    }

    if (menu.y + menu.height > screen->height()) {
      menu.y_shift = screen->height() - menu.height -
        screen->getBorderWidth();
      shifted = True;
    } else if (menu.y + (signed) menu.title_h < 0) {
      menu.y_shift = -screen->getBorderWidth();
      shifted = True;
    }

    if (shifted)
      XMoveWindow(*BaseDisplay::instance(), menu.window, menu.x_shift, menu.y_shift);

    if (which_sub != -1) {
      BasemenuItem *tmp = menuitems->find(which_sub);
      if (tmp->submenu()->isVisible()) {
	int sbl = (ce->x / menu.item_w), i = (ce->y / menu.item_h),
	  w = (sbl * menu.persub) + i;

	if (w != which_sub && (! tmp->submenu()->isTorn())) {
	  tmp->submenu()->internal_hide();

	  drawItem(which_sub, False, True);
	  which_sub = -1;
	}
      }
    }
  }
}


void Basemenu::leaveNotifyEvent(XCrossingEvent *ce) {
  if (ce->window == menu.frame) {
    if (which_press != -1 && which_sbl != -1 && menuitems->count() > 0) {
      int p = (which_sbl * menu.persub) + which_press;

      drawItem(p, (p == which_sub), True);

      which_sbl = which_press = -1;
    }

    if (shifted) {
      XMoveWindow(*BaseDisplay::instance(), menu.window, menu.x, menu.y);
      shifted = False;

      if (which_sub != -1) drawSubmenu(which_sub);
    }
  }
}


void Basemenu::reconfigure(void) {
  XSetWindowBackground(*BaseDisplay::instance(), menu.window,
		       screen->getBorderColor()->pixel());
  XSetWindowBorder(*BaseDisplay::instance(), menu.window,
		   screen->getBorderColor()->pixel());
  XSetWindowBorderWidth(*BaseDisplay::instance(), menu.window, screen->getBorderWidth());

  menu.bevel_w = screen->getBevelWidth();
  update();
}

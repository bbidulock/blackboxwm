// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Windowmenu.cc for Blackbox - an X11 Window manager
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

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "Windowmenu.hh"
#include "Screen.hh"
#include "Window.hh"
#include "Workspace.hh"


class SendToMenu : public Basemenu
{
public:
    SendToMenu( int src, BlackboxWindow *w );

    void refresh();

    void itemClicked( const Point &, const Item &, int );

private:
    BlackboxWindow *window;
};

SendToMenu::SendToMenu( int scr, BlackboxWindow *w )
    : Basemenu( scr ), window( w )
{
}

void SendToMenu::refresh()
{
  int i;
  clear();
  for ( i = 0; i < window->getScreen()->getCount(); i++ )
    insert( window->getScreen()->getWorkspace(i)->getName() );
  setItemChecked( window->getWorkspaceNumber(), true );
}

void SendToMenu::itemClicked( const Point &, const Item &item , int button )
{
    if ( button > 2 )
	return;
    if ( item.index() <= window->getScreen()->getCount()) {
	if ( item.index() == window->getScreen()->getCurrentWorkspaceID())
	    return;
	if ( window->isStuck() )
	    window->stick();

	if ( button == 1 )
	    window->withdraw();
	window->getScreen()->reassociateWindow( window, item.index(), True );
	if ( button == 2 )
	    window->getScreen()->changeWorkspaceID( item.index() );
    }
}

/*
  item indexes:

  0  - send to ...
  2  - shade
  3  - iconify
  4  - maximize
  5  - raise
  6  - lower
  7  - stick
  9  - kill client
  10 - close
*/
Windowmenu::Windowmenu( int scr, BlackboxWindow *w )
    : Basemenu( scr ), window( w )
{
    sendto = new SendToMenu( scr, w );

    insert( "Send To ...", sendto );
    insertSeparator();
    insert( "Shade", Shade );
    insert( "Iconify", Iconify );
    insert( "Maximize", Maximize );
    insert( "Raise", Raise );
    insert( "Lower", Lower );
    insert( "Stick", Stick );
    insertSeparator();
    insert( "Kill Client", KillClient );
    insert( "Close", Close );

    setItemEnabled( 5, false ); // raise
    setItemEnabled( 6, false ); // lower
    setItemEnabled( 9, false ); // kill client
}

Windowmenu::~Windowmenu()
{
    delete sendto;
}

void Windowmenu::itemClicked( const Point &, const Item &item, int button )
{
    if ( button != 1 )
	return;

    switch( item.function() ) {
    case Shade:	     window->shade();            break;
    case Iconify:    window->iconify();          break;
    case Maximize:   window->maximize( button ); break;
    case Raise:      window->raise();            break;
    case Lower:      window->lower();            break;
    case Stick:      window->stick();            break;
    case KillClient: window->killClient();       break;
    case Close:      window->close();            break;
    default: break;
    }
}

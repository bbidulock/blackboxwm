# src/Makefile.am for Blackbox 0.40.x beta - an X11 Window manager
# Copyright (c) 1997, 1998 by Brad Hughes, bhughes@tcac.net
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#  (See the included file COPYING / GPL-2.0)
#

CPPFLAGS =		@CPPFLAGS@				\
			@SHAPE@					\
			-DDEFAULTRC=\"$(pkgdatadir)/rc\"	\
			-DDEFAULTMENU=\"$(pkgdatadir)/menu\"	\
			-DDEFAULTSTYLE=\"$(pkgdatadir)/styles/default\"

bin_PROGRAMS =		blackbox

blackbox_SOURCES =	Basemenu.cc		\
			Clientmenu.cc		\
			Icon.cc			\
			Image.cc		\
			LinkedList.cc		\
			Rootmenu.cc		\
			Screen.cc		\
			Toolbar.cc		\
			Window.cc		\
			Windowmenu.cc		\
			Workspace.cc		\
			Workspacemenu.cc	\
			blackbox.cc		\
			main.cc

MAINTAINERCLEANFILES =	Makefile.in

distclean-local:
	rm -f *\~

# local dependencies

Basemenu.o: Basemenu.cc ../config.h blackbox.hh ../version.h Image.hh \
 LinkedList.hh Basemenu.hh Screen.hh
Clientmenu.o: Clientmenu.cc ../config.h blackbox.hh ../version.h \
 Image.hh LinkedList.hh Clientmenu.hh Basemenu.hh Screen.hh Toolbar.hh \
 Window.hh Workspace.hh Workspacemenu.hh
Icon.o: Icon.cc ../config.h blackbox.hh ../version.h Image.hh \
 LinkedList.hh Icon.hh Basemenu.hh Screen.hh Window.hh Workspace.hh \
 Clientmenu.hh Toolbar.hh
Image.o: Image.cc ../config.h blackbox.hh ../version.h Image.hh \
 LinkedList.hh Screen.hh
LinkedList.o: LinkedList.cc LinkedList.hh
Rootmenu.o: Rootmenu.cc ../config.h blackbox.hh ../version.h Image.hh \
 LinkedList.hh Rootmenu.hh Basemenu.hh Screen.hh
Screen.o: Screen.cc ../config.h blackbox.hh ../version.h Image.hh \
 LinkedList.hh Icon.hh Basemenu.hh Screen.hh Rootmenu.hh Toolbar.hh \
 Window.hh Workspace.hh Clientmenu.hh Workspacemenu.hh
Toolbar.o: Toolbar.cc ../config.h blackbox.hh ../version.h Image.hh \
 LinkedList.hh Icon.hh Basemenu.hh Rootmenu.hh Screen.hh Toolbar.hh \
 Workspace.hh Clientmenu.hh Window.hh Workspacemenu.hh
Window.o: Window.cc ../config.h blackbox.hh ../version.h Image.hh \
 LinkedList.hh Icon.hh Basemenu.hh Screen.hh Window.hh Windowmenu.hh \
 Workspace.hh Clientmenu.hh Toolbar.hh
Windowmenu.o: Windowmenu.cc ../config.h blackbox.hh ../version.h \
 Image.hh LinkedList.hh Screen.hh Window.hh Windowmenu.hh Basemenu.hh \
 Workspace.hh Clientmenu.hh Toolbar.hh
Workspace.o: Workspace.cc ../config.h blackbox.hh ../version.h \
 Image.hh LinkedList.hh Screen.hh Toolbar.hh Window.hh Workspace.hh \
 Clientmenu.hh Basemenu.hh Windowmenu.hh
Workspacemenu.o: Workspacemenu.cc ../config.h blackbox.hh ../version.h \
 Image.hh LinkedList.hh Screen.hh Workspacemenu.hh Basemenu.hh \
 Workspace.hh Clientmenu.hh Toolbar.hh Window.hh
blackbox.o: blackbox.cc ../config.h blackbox.hh ../version.h Image.hh \
 LinkedList.hh Basemenu.hh Rootmenu.hh Screen.hh Toolbar.hh Window.hh \
 Workspace.hh Clientmenu.hh
main.o: main.cc ../config.h blackbox.hh ../version.h Image.hh \
 LinkedList.hh
# src/Makefile.am for Blackbox - an X11 Window manager
# Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the 
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software. 
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
# DEALINGS IN THE SOFTWARE.

DEFAULT_MENU=$(pkgdatadir)/menu
DEFAULT_STYLE=$(pkgdatadir)/styles/Results

CPPFLAGS= @CPPFLAGS@ @SHAPE@ @ORDEREDPSEUDO@ \
@DEBUG@ @NLS@ @TIMEDCACHE@ \
-DLOCALEPATH=\"$(pkgdatadir)/nls\" \
-DDEFAULTMENU=\"$(DEFAULT_MENU)\" \
-DDEFAULTSTYLE=\"$(DEFAULT_STYLE)\" \
-I../lib

bin_PROGRAMS= blackbox

blackbox_SOURCES= Basemenu.cc Clientmenu.cc Configmenu.cc Iconmenu.cc Netizen.cc Rootmenu.cc Screen.cc Slit.cc Toolbar.cc Window.cc Windowmenu.cc Workspace.cc Workspacemenu.cc blackbox.cc main.cc
blackbox_LDADD= -L../lib -lblackbox

MAINTAINERCLEANFILES= Makefile.in

distclean-local:
	rm -f *\~ *.orig *.rej .\#*

# local dependencies

Basemenu.o: Basemenu.cc ../config.h ../lib/i18n.hh ../nls/blackbox-nls.hh \
 blackbox.hh ../lib/BaseDisplay.hh ../lib/Timer.hh Basemenu.hh ../lib/GCCache.hh \
 ../lib/Color.hh ../lib/Image.hh Screen.hh ../lib/Texture.hh ../lib/Util.hh \
 Configmenu.hh Iconmenu.hh Netizen.hh Rootmenu.hh Workspace.hh Workspacemenu.hh
Clientmenu.o: Clientmenu.cc ../config.h blackbox.hh ../lib/i18n.hh \
 ../nls/blackbox-nls.hh ../lib/BaseDisplay.hh ../lib/Timer.hh Clientmenu.hh \
 Basemenu.hh Screen.hh ../lib/Color.hh ../lib/Texture.hh ../lib/Util.hh \
 Configmenu.hh Iconmenu.hh Netizen.hh Rootmenu.hh Workspace.hh \
 Workspacemenu.hh Window.hh Windowmenu.hh
Configmenu.o: Configmenu.cc ../config.h ../lib/i18n.hh ../nls/blackbox-nls.hh \
 Configmenu.hh Basemenu.hh ../lib/Image.hh ../lib/Timer.hh ../lib/BaseDisplay.hh \
 ../lib/Color.hh Toolbar.hh Screen.hh ../lib/Texture.hh ../lib/Util.hh \
 Iconmenu.hh Netizen.hh Rootmenu.hh Workspace.hh Workspacemenu.hh blackbox.hh\
 Window.hh Windowmenu.hh
Iconmenu.o: Iconmenu.cc ../config.h ../lib/i18n.hh ../nls/blackbox-nls.hh \
 Iconmenu.hh Basemenu.hh Screen.hh ../lib/Color.hh ../lib/Texture.hh \
 ../lib/Util.hh Configmenu.hh Netizen.hh Rootmenu.hh ../lib/Timer.hh \
 Workspace.hh Workspacemenu.hh blackbox.hh ../lib/BaseDisplay.hh Window.hh \
 Windowmenu.hh
Netizen.o: Netizen.cc ../config.h Netizen.hh Screen.hh ../lib/Color.hh \
 ../lib/Texture.hh ../lib/Util.hh Configmenu.hh Basemenu.hh Iconmenu.hh \
 Rootmenu.hh ../lib/Timer.hh Workspace.hh Workspacemenu.hh blackbox.hh \
 ../lib/i18n.hh ../nls/blackbox-nls.hh ../lib/BaseDisplay.hh
Rootmenu.o: Rootmenu.cc ../config.h blackbox.hh ../lib/i18n.hh \
 ../nls/blackbox-nls.hh ../lib/BaseDisplay.hh ../lib/Timer.hh Rootmenu.hh \
 Basemenu.hh Screen.hh ../lib/Color.hh ../lib/Texture.hh ../lib/Util.hh \
 Configmenu.hh Iconmenu.hh Netizen.hh Workspace.hh Workspacemenu.hh
Screen.o: Screen.cc ../config.h ../lib/i18n.hh ../nls/blackbox-nls.hh \
 blackbox.hh ../lib/BaseDisplay.hh ../lib/Timer.hh Clientmenu.hh Basemenu.hh \
 ../lib/GCCache.hh ../lib/Color.hh Iconmenu.hh ../lib/Image.hh Screen.hh \
 ../lib/Texture.hh ../lib/Util.hh Configmenu.hh Netizen.hh Rootmenu.hh \
 Workspace.hh Workspacemenu.hh Slit.hh Toolbar.hh Window.hh Windowmenu.hh
Slit.o: Slit.cc ../config.h ../lib/i18n.hh ../nls/blackbox-nls.hh blackbox.hh \
 ../lib/BaseDisplay.hh ../lib/Timer.hh ../lib/Image.hh ../lib/Color.hh Screen.hh \
 ../lib/Texture.hh ../lib/Util.hh Configmenu.hh Basemenu.hh Iconmenu.hh \
 Netizen.hh Rootmenu.hh Workspace.hh Workspacemenu.hh Slit.hh Toolbar.hh
Toolbar.o: Toolbar.cc ../config.h ../lib/i18n.hh ../nls/blackbox-nls.hh \
 blackbox.hh ../lib/BaseDisplay.hh ../lib/Timer.hh Clientmenu.hh Basemenu.hh \
 ../lib/GCCache.hh ../lib/Color.hh Iconmenu.hh ../lib/Image.hh Rootmenu.hh \
 Screen.hh ../lib/Texture.hh ../lib/Util.hh Configmenu.hh Netizen.hh \
 Workspace.hh Workspacemenu.hh Toolbar.hh Window.hh Windowmenu.hh Slit.hh
Window.o: Window.cc ../config.h ../lib/i18n.hh ../nls/blackbox-nls.hh \
 blackbox.hh ../lib/BaseDisplay.hh ../lib/Timer.hh ../lib/GCCache.hh \
 ../lib/Color.hh Iconmenu.hh Basemenu.hh ../lib/Image.hh Screen.hh \
 ../lib/Texture.hh ../lib/Util.hh Configmenu.hh Netizen.hh Rootmenu.hh \
 Workspace.hh Workspacemenu.hh Toolbar.hh Window.hh Windowmenu.hh Slit.hh
Windowmenu.o: Windowmenu.cc ../config.h ../lib/i18n.hh ../nls/blackbox-nls.hh \
 blackbox.hh ../lib/BaseDisplay.hh ../lib/Timer.hh Screen.hh ../lib/Color.hh \
 ../lib/Texture.hh ../lib/Util.hh Configmenu.hh Basemenu.hh Iconmenu.hh \
 Netizen.hh Rootmenu.hh Workspace.hh Workspacemenu.hh Window.hh Windowmenu.hh
Workspace.o: Workspace.cc ../config.h ../lib/i18n.hh ../nls/blackbox-nls.hh \
 blackbox.hh ../lib/BaseDisplay.hh ../lib/Timer.hh Clientmenu.hh Basemenu.hh \
 Netizen.hh Screen.hh ../lib/Color.hh ../lib/Texture.hh ../lib/Util.hh \
 Configmenu.hh Iconmenu.hh Rootmenu.hh Workspace.hh Workspacemenu.hh \
 Toolbar.hh Window.hh Windowmenu.hh
Workspacemenu.o: Workspacemenu.cc ../config.h ../lib/i18n.hh \
 ../nls/blackbox-nls.hh blackbox.hh ../lib/BaseDisplay.hh ../lib/Timer.hh Screen.hh \
 ../lib/Color.hh ../lib/Texture.hh ../lib/Util.hh Configmenu.hh Basemenu.hh \
 Iconmenu.hh Netizen.hh Rootmenu.hh Workspace.hh Workspacemenu.hh Toolbar.hh
blackbox.o: blackbox.cc ../config.h ../lib/i18n.hh ../nls/blackbox-nls.hh \
 blackbox.hh ../lib/BaseDisplay.hh ../lib/Timer.hh Basemenu.hh Clientmenu.hh \
 ../lib/GCCache.hh ../lib/Color.hh ../lib/Image.hh Rootmenu.hh Screen.hh \
 ../lib/Texture.hh ../lib/Util.hh Configmenu.hh Iconmenu.hh Netizen.hh \
 Workspace.hh Workspacemenu.hh Slit.hh Toolbar.hh Window.hh Windowmenu.hh
main.o: main.cc ../version.h ../config.h ../lib/i18n.hh \
 ../nls/blackbox-nls.hh blackbox.hh ../lib/BaseDisplay.hh ../lib/Timer.hh

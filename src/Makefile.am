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
-I$(top_srcdir)/lib

bin_PROGRAMS= blackbox

blackbox_SOURCES= Basemenu.cc Clientmenu.cc Configmenu.cc Iconmenu.cc Rootmenu.cc Screen.cc Slit.cc Toolbar.cc Window.cc Windowmenu.cc Workspace.cc Workspacemenu.cc blackbox.cc main.cc
blackbox_DEPENDENCIES= $(top_srcdir)/lib/libblackbox.a
blackbox_LDADD= -L$(top_srcdir)/lib -lblackbox

MAINTAINERCLEANFILES= Makefile.in

distclean-local:
	rm -f *\~ *.orig *.rej .\#*

# local dependencies

Basemenu.o: Basemenu.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh $(top_srcdir)/nls/blackbox-nls.hh \
 blackbox.hh $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh Basemenu.hh $(top_srcdir)/lib/GCCache.hh \
 $(top_srcdir)/lib/Color.hh $(top_srcdir)/lib/Image.hh Screen.hh $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh \
 Configmenu.hh Iconmenu.hh Rootmenu.hh Workspace.hh Workspacemenu.hh
Clientmenu.o: Clientmenu.cc $(top_srcdir)/config.h blackbox.hh $(top_srcdir)/lib/i18n.hh \
 $(top_srcdir)/nls/blackbox-nls.hh $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh Clientmenu.hh \
 Basemenu.hh Screen.hh $(top_srcdir)/lib/Color.hh $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh \
 Configmenu.hh Iconmenu.hh Rootmenu.hh Workspace.hh \
 Workspacemenu.hh Window.hh Windowmenu.hh
Configmenu.o: Configmenu.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh $(top_srcdir)/nls/blackbox-nls.hh \
 Configmenu.hh Basemenu.hh $(top_srcdir)/lib/Image.hh $(top_srcdir)/lib/Timer.hh $(top_srcdir)/lib/BaseDisplay.hh \
 $(top_srcdir)/lib/Color.hh Toolbar.hh Screen.hh $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh \
 Iconmenu.hh Rootmenu.hh Workspace.hh Workspacemenu.hh blackbox.hh\
 Window.hh Windowmenu.hh
Iconmenu.o: Iconmenu.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh $(top_srcdir)/nls/blackbox-nls.hh \
 Iconmenu.hh Basemenu.hh Screen.hh $(top_srcdir)/lib/Color.hh $(top_srcdir)/lib/Texture.hh \
 $(top_srcdir)/lib/Util.hh Configmenu.hh Rootmenu.hh $(top_srcdir)/lib/Timer.hh \
 Workspace.hh Workspacemenu.hh blackbox.hh $(top_srcdir)/lib/BaseDisplay.hh Window.hh \
 Windowmenu.hh
Rootmenu.o: Rootmenu.cc $(top_srcdir)/config.h blackbox.hh $(top_srcdir)/lib/i18n.hh \
 $(top_srcdir)/nls/blackbox-nls.hh $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh Rootmenu.hh \
 Basemenu.hh Screen.hh $(top_srcdir)/lib/Color.hh $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh \
 Configmenu.hh Iconmenu.hh Workspace.hh Workspacemenu.hh
Screen.o: Screen.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh $(top_srcdir)/nls/blackbox-nls.hh \
 blackbox.hh $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh Clientmenu.hh Basemenu.hh \
 $(top_srcdir)/lib/GCCache.hh $(top_srcdir)/lib/Color.hh Iconmenu.hh $(top_srcdir)/lib/Image.hh Screen.hh \
 $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh Configmenu.hh $(top_srcdir)/lib/Netwm.hh \
 Rootmenu.hh Workspace.hh Workspacemenu.hh Slit.hh Toolbar.hh Window.hh \
 Windowmenu.hh
Slit.o: Slit.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh $(top_srcdir)/nls/blackbox-nls.hh blackbox.hh \
 $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh $(top_srcdir)/lib/Image.hh $(top_srcdir)/lib/Color.hh Screen.hh \
 $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh Configmenu.hh Basemenu.hh Iconmenu.hh \
 Rootmenu.hh Workspace.hh Workspacemenu.hh Slit.hh Toolbar.hh
Toolbar.o: Toolbar.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh $(top_srcdir)/nls/blackbox-nls.hh \
 blackbox.hh $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh Clientmenu.hh Basemenu.hh \
 $(top_srcdir)/lib/GCCache.hh $(top_srcdir)/lib/Color.hh Iconmenu.hh $(top_srcdir)/lib/Image.hh Rootmenu.hh \
 Screen.hh $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh Configmenu.hh \
 Workspace.hh Workspacemenu.hh Toolbar.hh Window.hh Windowmenu.hh Slit.hh
Window.o: Window.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh $(top_srcdir)/nls/blackbox-nls.hh \
 blackbox.hh $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh $(top_srcdir)/lib/GCCache.hh \
 $(top_srcdir)/lib/Color.hh Iconmenu.hh Basemenu.hh $(top_srcdir)/lib/Image.hh Screen.hh \
 $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh Configmenu.hh Rootmenu.hh \
 Workspace.hh Workspacemenu.hh Toolbar.hh Window.hh Windowmenu.hh Slit.hh
Windowmenu.o: Windowmenu.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh $(top_srcdir)/nls/blackbox-nls.hh \
 blackbox.hh $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh Screen.hh $(top_srcdir)/lib/Color.hh \
 $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh Configmenu.hh Basemenu.hh Iconmenu.hh \
 Rootmenu.hh Workspace.hh Workspacemenu.hh Window.hh Windowmenu.hh
Workspace.o: Workspace.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh $(top_srcdir)/nls/blackbox-nls.hh \
 blackbox.hh $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh Clientmenu.hh Basemenu.hh \
 Screen.hh $(top_srcdir)/lib/Color.hh $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh \
 Configmenu.hh Iconmenu.hh Rootmenu.hh Workspace.hh Workspacemenu.hh \
 Toolbar.hh Window.hh Windowmenu.hh
Workspacemenu.o: Workspacemenu.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh \
 $(top_srcdir)/nls/blackbox-nls.hh blackbox.hh $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh Screen.hh \
 $(top_srcdir)/lib/Color.hh $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh Configmenu.hh Basemenu.hh \
 Iconmenu.hh Rootmenu.hh Workspace.hh Workspacemenu.hh Toolbar.hh
blackbox.o: blackbox.cc $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh $(top_srcdir)/nls/blackbox-nls.hh \
 blackbox.hh $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh Basemenu.hh Clientmenu.hh \
 $(top_srcdir)/lib/GCCache.hh $(top_srcdir)/lib/Color.hh $(top_srcdir)/lib/Image.hh Rootmenu.hh Screen.hh \
 $(top_srcdir)/lib/Texture.hh $(top_srcdir)/lib/Util.hh Configmenu.hh Iconmenu.hh \
 Workspace.hh Workspacemenu.hh Slit.hh Toolbar.hh Window.hh Windowmenu.hh
main.o: main.cc $(top_srcdir)/version.h $(top_srcdir)/config.h $(top_srcdir)/lib/i18n.hh \
 $(top_srcdir)/nls/blackbox-nls.hh blackbox.hh $(top_srcdir)/lib/BaseDisplay.hh $(top_srcdir)/lib/Timer.hh

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

CPPFLAGS= @CPPFLAGS@ @SHAPE@ @SLIT@ @INTERLACE@ @ORDEREDPSEUDO@ \
@DEBUG@ @NEWWMSPEC@ @NLS@ @TIMEDCACHE@ \
-DLOCALEPATH=\"$(pkgdatadir)/nls\" \
-DDEFAULTMENU=\"$(DEFAULT_MENU)\" \
-DDEFAULTSTYLE=\"$(DEFAULT_STYLE)\" \
-Wall -W -fno-exceptions

bin_PROGRAMS= blackbox

blackbox_SOURCES= BaseDisplay.cc Basemenu.cc Clientmenu.cc Color.cc Configmenu.cc GCCache.cc Iconmenu.cc Image.cc LinkedList.cc Netizen.cc Rootmenu.cc Screen.cc Slit.cc Texture.cc Timer.cc Toolbar.cc Util.cc Widget.cc Window.cc Windowmenu.cc Workspace.cc Workspacemenu.cc blackbox.cc bsd-snprintf.c i18n.cc main.cc

MAINTAINERCLEANFILES= Makefile.in

distclean-local:
	rm -f *\~ *.orig *.rej

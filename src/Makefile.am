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


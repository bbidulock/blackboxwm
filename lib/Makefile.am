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

CPPFLAGS= @CPPFLAGS@ @SHAPE@ @ORDEREDPSEUDO@ \
@DEBUG@ @NLS@ @TIMEDCACHE@ \
-DLOCALEPATH=\"$(pkgdatadir)/nls\"

LIBRARIES = libblackbox.a
noinst_LIBRARIES = libblackbox.a
libblackbox_a_SOURCES= BaseDisplay.cc Color.cc GCCache.cc Image.cc ImageControl.cc Netwm.cc Texture.cc Timer.cc Util.cc i18n.cc

MAINTAINERCLEANFILES= Makefile.in

distclean-local:
	rm -f *\~ *.orig *.rej .\#*

# local dependencies

BaseDisplay.o: BaseDisplay.cc $(top_srcdir)/config.h i18n.hh \
 $(top_srcdir)/nls/blackbox-nls.hh BaseDisplay.hh Timer.hh GCCache.hh Color.hh \
 Util.hh
Color.o: Color.cc $(top_srcdir)/config.h Color.hh BaseDisplay.hh Timer.hh
GCCache.o: GCCache.cc $(top_srcdir)/config.h GCCache.hh BaseDisplay.hh Timer.hh \
 Color.hh Util.hh
Image.o: Image.cc $(top_srcdir)/config.h i18n.hh \
 $(top_srcdir)/nls/blackbox-nls.hh BaseDisplay.hh Timer.hh GCCache.hh Color.hh \
 Image.hh Texture.hh Util.hh
ImageControl.o: ImageControl.cc $(top_srcdir)/config.h i18n.hh \
 $(top_srcdir)/nls/blackbox-nls.hh BaseDisplay.hh Timer.hh Color.hh Image.hh \
 Texture.hh Util.hh
Netwm.o: Netwm.hh Netwm.cc
Texture.o: Texture.cc $(top_srcdir)/config.h Texture.hh Color.hh Util.hh \
 BaseDisplay.hh Timer.hh Image.hh i18n.hh $(top_srcdir)/nls/blackbox-nls.hh
Timer.o: Timer.cc $(top_srcdir)/config.h BaseDisplay.hh Timer.hh Util.hh
Util.o: Util.cc $(top_srcdir)/config.h Util.hh
i18n.o: i18n.cc $(top_srcdir)/config.h i18n.hh $(top_srcdir)/nls/blackbox-nls.hh

# src/Makefile.am for Blackbox - an X11 Window manager
# Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
# Copyright (c) 1997 - 2000, 2002 - 2005
#         Bradley T Hughes <bhughes at trolltech.com>
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

AM_CPPFLAGS = 		@SHAPE@ @MITSHM@ @XFT@ @DEBUG@ @NLS@ \
			-DLOCALEPATH=\"$(pkgdatadir)/nls\"
lib_LTLIBRARIES = 	libbt.la
libbt_la_SOURCES = 	Application.cc					\
			Bitmap.cc					\
			Color.cc					\
			Display.cc					\
			EWMH.cc						\
			Font.cc						\
			Image.cc					\
			Menu.cc						\
			Pen.cc						\
			PixmapCache.cc					\
			Rect.cc						\
			Resource.cc					\
			Texture.cc					\
			Timer.cc					\
			Unicode.cc					\
			Util.cc						\
			XDG.cc

libbtdir = $(includedir)/bt
libbt_HEADERS = 	Application.hh					\
			Bitmap.hh					\
			Color.hh					\
			Display.hh					\
			EWMH.hh						\
			EventHandler.hh					\
			Font.hh						\
			Image.hh					\
			Menu.hh						\
			Pen.hh						\
			PixmapCache.hh					\
			Rect.hh						\
			Resource.hh					\
			Texture.hh					\
			Timer.hh					\
			Unicode.hh					\
			Util.hh						\
			XDG.hh

libbt_la_LIBADD =	@ICONV@ @LOCALE@

pkgconfigdir = 		$(libdir)/pkgconfig
nodist_pkgconfig_DATA =	libbt.pc

DISTCLEANFILES = 	*\~ *.orig *.rej .\#* *.il *.pchi
MAINTAINERCLEANFILES = 	Makefile.in

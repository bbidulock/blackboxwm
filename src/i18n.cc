// i18n.cc for Blackbox - an X11 Window manager
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

#include "i18n.hh"

#include <X11/Xlocale.h>

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#  include <stdio.h>
#endif // STDC_HEADERS

#ifdef    HAVE_LOCALE_H
#  include <locale.h>
#endif // HAVE_LOCALE_H


I18n *i18n;

void NLSInit(void) {
  i18n = new I18n;
  i18n->openCatalog();
}


I18n::I18n(void) {
#ifdef    HAVE_SETLOCALE
  locale = setlocale(LC_ALL, "");
  if (! locale) {
    fprintf(stderr, "failed to set locale, reverting to \"C\"\n");
#endif // HAVE_SETLOCALE

    locale = "C";

#ifdef    HAVE_SETLOCALE
  }
#endif // HAVE_SETLOCALE

  // truncate any encoding off the end of the locale
  char *l = strchr(locale, '.');
  if (l) *l = '\0';

  int lp = strlen(LOCALEPATH), lc = strlen(locale),
      ct = strlen("//blackbox.cat"), len = lp + lc + ct + 1;
  catalog_filename = new char[len];
  strncpy(catalog_filename, LOCALEPATH, lp);
  *(catalog_filename + lp) = '/';
  strncpy(catalog_filename + lp + 1, locale, lc);
  strncpy(catalog_filename + lp + lc + 1, "/blackbox.cat", ct);
}


I18n::~I18n(void) {
  delete [] catalog_filename;

#if defined(NLS) && defined(HAVE_CATCLOSE)
  if (catalog_fd != (nl_catd) -1)
    catclose(catalog_fd);
#endif // HAVE_CATCLOSE
}


void I18n::openCatalog(void) {
#if defined(NLS) && defined(HAVE_CATOPEN)
#  ifdef    MCLoadBySet
  catalog_fd = catopen(catalog_filename, MCLoadBySet);
#  else // !MCLoadBySet
  catalog_fd = catopen(catalog_filename, NL_CAT_LOCALE);
#  endif // MCLoadBySet
  
  if (catalog_fd == (nl_catd) -1)
    fprintf(stderr, "failed to open catalog, using default messages\n");
#else // !HAVE_CATOPEN
  catalog_fd = (nl_catd) -1;
#endif // HAVE_CATOPEN
}


const char *I18n::getMessage(int set, int msg, const char *s) {
#if   defined(NLS) && defined(HAVE_CATGETS)
  if (catalog_fd != (nl_catd) -1)
    return (const char *) catgets(catalog_fd, set, msg, s);
  else
#endif
    return s;
}

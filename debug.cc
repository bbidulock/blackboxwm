//
// debug.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// (See the included file COPYING / GPL-2.0)
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>

#include "debug.hh"


Debugger::Debugger(void) {
  fhLog = stderr;
  level = 0;
  disable();
  prefix = '+';
}

Debugger::Debugger(char pfx) {
  fhLog = stderr;
  level = 0;
  disable();
  prefix = pfx;
}

Debugger::Debugger(char *file) {
  level = 0;
  if (file == NULL) {
    fhLog = stderr;
    return;
  }
  if ((fhLog = fopen(file, "a+")) == NULL) {
    fhLog = stderr;
    return;
  }
  disable();
  prefix = '+';
}

Debugger::Debugger(char pfx, char *file) {
  level = 0;
  if (file == NULL) {
    fhLog = stderr;
    return;
  }
  if ((fhLog = fopen(file, "a+")) == NULL) {
    fhLog = stderr;
    return;
  }
  disable();
  prefix = pfx;
}

Debugger::~Debugger(void) {
  if (fhLog != stderr) fclose(fhLog);
}

void Debugger::enable(void) {
  enabled = 1;
}

void Debugger::disable(void) {
  enabled = 0;
}

void Debugger::enter(void) {
  if (enabled) {
    level++;
    if (level > 255) abort();
  }
}

void Debugger::enter(char *txt, ...) {
  if (enabled) {
    char buf[1024];
    va_list ap;
    
    va_start(ap, txt);
    vsnprintf(buf, 1023, txt, ap);
    va_end(ap);
    write(buf);
    level++;
    if (level > 255) abort();
  }
}

void Debugger::leave(void) {
  if (enabled) {
    level--;
    write("done\n");
  }
}

void Debugger::leave(char *txt, ...) {
  if (enabled) {
    char buf[1024];
    va_list ap;
    
    level--;
    va_start(ap, txt);
    vsnprintf(buf, 1023, txt, ap);
    va_end(ap);
    write(buf);
  }
}

void Debugger::msg(char *txt, ...) {
  if (enabled) {
    char buf[1024];
    va_list ap;
    
    va_start(ap, txt);
    vsnprintf(buf, 1023, txt, ap);
    va_end(ap);
    write(buf);
  }
}

void Debugger::write(char *txt) {
  if (enabled) {
    int i;
    
    for (i = 0; i < level; i++) fputc(prefix, fhLog);
    if (level) fputc(' ', fhLog);
    fputs(txt, fhLog);
  }
}

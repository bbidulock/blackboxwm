/*----------------------------------------------------------------------------
 * (C) 1997-1998 Armin Biere 
 *
 *     $Id: assert.c,v 1.1 2001/11/30 07:54:36 shaleh Exp $
 *----------------------------------------------------------------------------
 */

#include "config.h"

/* ------------------------------------------------------------------------ */

#include <unistd.h>
#include <signal.h>
#include <stdio.h>

/* ------------------------------------------------------------------------ */

void _failed_assertion(char * file, int lineno)
{
  fprintf(stderr, "*** %s:%d: assertion failed!\n", file, lineno);
  fflush(stderr); 
  kill(getpid(),SIGSEGV);
}

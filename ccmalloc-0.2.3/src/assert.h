/*----------------------------------------------------------------------------
 * (C) 1997-1998 Armin Biere 
 *
 *     $Id: assert.h,v 1.1 2001/11/30 07:54:36 shaleh Exp $
 *----------------------------------------------------------------------------
 */

#ifndef _assert_h_INCLUDED
#define _assert_h_INCLUDED

#ifdef DEBUG

extern void _failed_assertion (char * str, int lineno);
#define assert(b) {if(!(b))_failed_assertion(__FILE__,__LINE__);}
#else
#define assert(b)
#endif

#endif

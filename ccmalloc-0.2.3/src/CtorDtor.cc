/*----------------------------------------------------------------------------
 * (C) 1997-1998 Armin Biere 
 *
 *     $Id: CtorDtor.cc,v 1.1.1.1 2001/11/30 07:54:36 shaleh Exp $
 *----------------------------------------------------------------------------
 */

extern "C" {
#include "ccmalloc.h"
};

class CCMalloc_InitAndReport
{
public:
  CCMalloc_InitAndReport() { ccmalloc_init(); }
  ~CCMalloc_InitAndReport() { ccmalloc_report(); }
};

static CCMalloc_InitAndReport ccmalloc_initAndReport;

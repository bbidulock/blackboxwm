#ifndef   __mem_h
#define   __mem_h

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H


#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

  
  static unsigned long totalbytes;

  static inline void allocate(unsigned long b, char *func) {
    totalbytes += b;
    fprintf(stderr, "%s:   allocate: %8lu/%8lu\n", func, b, totalbytes);
  }
  
  static inline void deallocate(unsigned long b, char *func) {
    totalbytes -= b;
    fprintf(stderr, "%s: deallocate: %8lu/%8lu\n", func, b, totalbytes);
  }
  
  
#ifdef    __cplusplus
};
#endif // __cplusplus


#endif // __mem_h 

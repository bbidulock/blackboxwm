/*----------------------------------------------------------------------------
 * (C) 1997-1998 Armin Biere 
 *
 *     $Id: wrapper.c,v 1.1 2001/11/30 07:54:36 shaleh Exp $
 *----------------------------------------------------------------------------
 */

#include "config.h"

/* ------------------------------------------------------------------------ */

/* Uncomment the next line to get a raw logging of all calls to malloc
 * and free.
 */

/*
#define LOG_EXTERNAL_MALLOC
*/

/* ------------------------------------------------------------------------ */

#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef OS_IS_SUNOS
/* For `memset()' under SunOS 4.3.1 */
#include <memory.h>
#endif

#ifdef DEBUG
#include <signal.h>
#endif

extern void * ccmalloc_malloc(size_t);
extern void ccmalloc_free(void *);
extern int ccmalloc_size(void *, size_t *);
extern void ccmalloc_die_or_continue(char * fmt, ...);

#ifndef RTLD_NOW
#define RTLD_NOW 1
#endif

#define MALLOC_SYMBOL "malloc"
#define FREE_SYMBOL "free"

/* ------------------------------------------------------------------------ */
/* This should work. But sometimes it is not enough. Then you have to set
 * this to a higher value and recompile ccmalloc.
 */

#define MAX_STATIC_MEM 10000

/* ------------------------------------------------------------------------ */

enum WrapperState
{
  WRAPPER_UNINITIALIZED,
  WRAPPER_INITIALIZING,
  WRAPPER_INITIALIZED
};

/* ------------------------------------------------------------------------ */

static void * (*libc_malloc)(size_t) = 0;
static void (*libc_free)(void *) = 0;
static enum WrapperState wrapper_state = WRAPPER_UNINITIALIZED;
static int semaphore = 0;

static char static_mem[MAX_STATIC_MEM];
static char *bottom_static_mem = static_mem;
static char *top_static_mem = static_mem + MAX_STATIC_MEM;

static long wrapper_bytes_allocated = 0,
	    wrapper_num_allocated = 0, wrapper_num_deallocated = 0;

/* ------------------------------------------------------------------------ */

static int wrapper_strlen(const char * str)
{
  const char *p;
  for(p=str; *p; p++)
    ;

  return p - str;
}

/* ------------------------------------------------------------------------ */
/* can not call `printf' because it calls malloc itself */

static void print(const char * str)
{
  write(2, str, wrapper_strlen(str));
}

/* ------------------------------------------------------------------------ */

#ifdef LOG_EXTERNAL_MALLOC

static char print_hex_tab[16] =
{
  '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
};

/* 32Bit: sizeof(unsigned) == 4 */

static void print_hex(unsigned d)
{
  char buffer[9];

  buffer[7] = print_hex_tab [ d % 16 ]; d /= 16;
  buffer[6] = print_hex_tab [ d % 16 ]; d /= 16;
  buffer[5] = print_hex_tab [ d % 16 ]; d /= 16;
  buffer[4] = print_hex_tab [ d % 16 ]; d /= 16;
  buffer[3] = print_hex_tab [ d % 16 ]; d /= 16;
  buffer[2] = print_hex_tab [ d % 16 ]; d /= 16;
  buffer[1] = print_hex_tab [ d % 16 ]; d /= 16;
  buffer[0] = print_hex_tab [ d % 16 ]; d /= 16;

  buffer[8] = 0;

  print(buffer);
}

static void print_dec(unsigned d)
{
  if(d)
    {
      char buffer[11];          /* also 32 bit !! */
      int i=10;

      buffer[11] = 0;

      while(1)
	{
	  buffer[i] = print_hex_tab [ d % 10 ]; d /= 10;
	  if(d) i--;
	  else break;
	}
      
      print(buffer + i);
    }
  else print("0");
}

#endif

/* ------------------------------------------------------------------------ */

static void load_libc()
{
  void * handle;
  const char * err; char * libcname;

#ifdef OS_IS_SUNOS
  {
    DIR * dir = opendir("/lib");
    if(dir)
      {
	libcname = LIBCNAME;

	for(;;)
	  {
	    struct dirent * e;
	    if((e = readdir(dir)) == NULL)
	      {
		closedir(dir);
		print("*** wrapper: could not find `libc.so'\n");
		exit(1);
	      }
	    else
	      {
		if(strncmp(libcname, e -> d_name, strlen(libcname)) == 0)
		  {
		    static char buffer[200];
		    strcpy(buffer, "/lib/");
		    libcname = strcat(buffer, e -> d_name);
		    break;
		  }
	      }
	  }
      }
    else
      {
	print("*** wrapper: can not open `/lib' directory\n");
	exit(1);
      }
  }
#else
  libcname = LIBCNAME;
#endif

  handle = dlopen(libcname, RTLD_NOW);
  if((err = dlerror()))
    {
      print("*** wrapper can not open `"); print(libcname); print("'!\n");
      print("*** dlerror() reports: "); print(err); print("\n");
      exit(1);
    }
  
  libc_malloc = (void*(*)(size_t)) dlsym(handle, MALLOC_SYMBOL);
  if((err = dlerror()))
    {
      print("*** wrapper does not find `");
      print(MALLOC_SYMBOL);
      print("' in `libc.so'!\n");
      exit(1);
    }

  libc_free = (void(*)(void*)) dlsym(handle, FREE_SYMBOL);
  if((err = dlerror()))
    {
      print("*** wrapper does not find `");
      print(FREE_SYMBOL);
      print("' in `libc.so'!\n");
      exit(1);
    }
}

/* ------------------------------------------------------------------------ */
/* just play it save and do 8 byte alignment
 */

#define WRAPPER_ALIGN_MASK 7

/* ------------------------------------------------------------------------ */

#define wrapper_isAligned(c) ((((unsigned)c) & WRAPPER_ALIGN_MASK) == 0)

#define wrapper_align(c)                                        \
{                                                               \
  if(!wrapper_isAligned(c))                                     \
    {                                                           \
      *(unsigned*)&c = (((unsigned)c) | WRAPPER_ALIGN_MASK);    \
      *(unsigned*)&c = (((unsigned)c) + 1);                     \
    }                                                           \
}

/* ------------------------------------------------------------------------ */

static void * static_malloc(size_t n)
{
  char * res;
  int sizeof_size_t;
  size_t * where_to_store_size, aligned_size;

  aligned_size = n;
  sizeof_size_t = sizeof(sizeof_size_t);

  wrapper_align(aligned_size);
  wrapper_align(sizeof_size_t);
  wrapper_align(bottom_static_mem);

  where_to_store_size = (size_t *) bottom_static_mem;
  bottom_static_mem += sizeof_size_t;   /* aligned + aligned = aligned */
  res = bottom_static_mem;
  bottom_static_mem += aligned_size;

  if(bottom_static_mem>=top_static_mem)
    {
      print("*** static memory in wrapper exceeded\n");
      exit(1);
    }
  
  *where_to_store_size = n;

  return (void*) res;
}

/* ------------------------------------------------------------------------ */

static void static_free(void * p)
{
  #ifdef DEBUG
   {
     if(((char*)p) < static_mem || ((char*)p) >= top_static_mem)
       {
	 print("*** nonvalid arg for static_free() in wrapper\n");
	 kill(0,SIGSEGV);               // simulate assert
	 exit(1);
       }
   }
  #else
    (void) p;
  #endif
}

/* ------------------------------------------------------------------------ */

void libcwrapper_inc_semaphore() { semaphore++; }
void libcwrapper_dec_semaphore() { semaphore--; }

/* ------------------------------------------------------------------------ */

void * malloc(size_t n)
{
  void * res;

  switch(wrapper_state)
    {
      case WRAPPER_UNINITIALIZED:

	wrapper_state = WRAPPER_INITIALIZING;
	load_libc();
	wrapper_state = WRAPPER_INITIALIZED;

	/* !!!!!!! fall through !!!!!!! */
      
      case WRAPPER_INITIALIZED:

	if(semaphore)
	  {
	    wrapper_num_allocated++;
	    wrapper_bytes_allocated += n;
	    res = (*libc_malloc)(n);
	  }
	else
	  {
	    semaphore++;
	    res = ccmalloc_malloc(n);
	    semaphore--;
	  }
	break;
      
      default:
      case WRAPPER_INITIALIZING:

	res = static_malloc(n);
	break;
    }

# ifdef LOG_EXTERNAL_MALLOC
    {
      print("::: 0x");
      print_hex((unsigned) res);
      if(wrapper_state == WRAPPER_INITIALIZING) print(" static  ");
      else if(semaphore) print(" internal");
      else print(" external");
      print(" malloc(");
      print_dec(n);
      print(")\n");
    }
# endif

#if 0
  memset(res, 0, n);
#endif
  
  return res;
}

/* ------------------------------------------------------------------------ */

void free(void * p)
{
# ifdef DEBUG
    {
      if(wrapper_state != WRAPPER_INITIALIZING)
	{
	  if(static_mem <= ((char*)p) && ((char*)p) < top_static_mem)
	    {
	      print("*** free() in wrapper called for argument\n");
	      print("*** that was allocated during initialization!\n");
	    }
	}
    }
# endif

  switch(wrapper_state)
    {
      case WRAPPER_UNINITIALIZED:

	print("*** free() in wrapper called before malloc()\n");
	exit(1);
	break;
      
      case WRAPPER_INITIALIZED:

	if(semaphore)
	  {
	    wrapper_num_deallocated++;
	    (*libc_free)(p);
	  }
	else
	  {
	    semaphore++;
	    ccmalloc_free(p);
	    semaphore--;
	  }
	break;
      
      default:
      case WRAPPER_INITIALIZING:
	
	static_free(p);
	break;
    }

# ifdef LOG_EXTERNAL_MALLOC
    {
      print("::: 0x");
      print_hex((unsigned) p);
      if(semaphore) print(" internal");
      else print(" external");
      print(" free\n");
    }
# endif
}

/* ------------------------------------------------------------------------ */

void * calloc(size_t nmemb, size_t size)
{
  void * res;
  size *= nmemb;
  res = malloc(size);
  memset(res, 0, size);
  return res;
}

/* ------------------------------------------------------------------------ */

static void * _realloc(void * ptr, size_t size, size_t previous_size)
{
  void * res;

  res = (void *) malloc(size);
  if(size < previous_size) (void) memcpy(res, ptr, size);
  else (void) memcpy(res, ptr, previous_size);
  free(ptr);

  return res;
}

/* ------------------------------------------------------------------------ */

void * realloc(void *ptr, size_t size)
{
  size_t previous_size, sizeof_size_t;
  char * where_size_was_stored;
  void * res;

  if(ptr == 0) res = malloc(size);
  else
  if(size == 0) 
    {
      free(ptr);
      res =  0;
    }
  else
    {
      switch(wrapper_state)
	{
	  default:
	    print("*** ccmalloc: non valid wrapper state in realloc()\n");
	    exit(1);
	    res = 0;
	    break;

	  case WRAPPER_UNINITIALIZED:
	    print("*** realloc() in wrapper called before malloc()\n");
	    exit(1);
	    res = 0;
	    break;

	  case WRAPPER_INITIALIZING:
	    sizeof_size_t = sizeof(size_t);
	    wrapper_align(sizeof_size_t);
	    where_size_was_stored = (char*) ptr;
	    where_size_was_stored -= sizeof_size_t;
	    previous_size = *(size_t*)where_size_was_stored;
	    res = _realloc(ptr, size, previous_size);
	    break;

	  case WRAPPER_INITIALIZED:
	    if(ccmalloc_size(ptr, &previous_size))
	      {
		res = _realloc(ptr, size, previous_size);
	      }
	    else
	      {
		ccmalloc_die_or_continue(
		  "*** realloc(0x%x) called with non valid argument\n",
		  (unsigned) ptr);
		res = malloc(size);
	      }
	    break;
	}
    }
  
  return res;
}

/* ------------------------------------------------------------------------ */

const char * wrapper_stats()
{
  static char buffer[200];
  sprintf(buffer, 
	  "%ld allocations, %ld deallocations, %ld bytes allocated",
	  wrapper_num_allocated, wrapper_num_deallocated,
	  wrapper_bytes_allocated);
  return buffer;
}

/*----------------------------------------------------------------------------
 * (C) 1997-1998 Armin Biere
 *     (original version)
 *
 *     1998 Johannes Keukelaar
 *     (dont-log-chain, only-log-chain, read-dynlib-with-gdb, fixes)
 *
 *     1998 Remy Didier
 *     (logpid)
 *
 *     $Id: callchain.c,v 1.1.1.1 2001/11/30 07:54:36 shaleh Exp $
 *----------------------------------------------------------------------------
 */

#include "config.h"

/* Compile time options (for debugging). Default is to leave them uncommented.
 */

/* USE_BIDIRECTIONAL_CODE enables some old not so efficient code.
 */
/*
#define USE_BIDIRECTIONAL_CODE
*/

/* Logs all communication with gdb pipe in the log file. But it does not
 * work if USE_BIDIRECTIONAL_CODE is enabled and I have no idea how to
 * fix this.
 */
/*
#define DEBUG_GDB_PIPE			
*/

/* The following flags control the blocking behaviour of the gdb pipe.
 * Only useful when USE_BIDIRECTIONAL_CODE is defined!
 */
/*
#define GDB_PIPE_CHILD_INPUT_IS_NON_BLOCKED
#define GDB_PIPE_CHILD_OUTPUT_IS_NON_BLOCKED
*/

/* ------------------------------------------------------------------------ */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef OS_IS_SOLARIS
#include <strings.h>	/* for rindex */
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#include "hash.h"
#include "assert.h"

extern void libcwrapper_inc_semaphore();
extern void libcwrapper_dec_semaphore();
extern const char * wrapper_stats();

/* ------------------------------------------------------------------------ */

typedef struct Symbol
{
  char * name;
  caddr_t addr;
}
Symbol;

/* ------------------------------------------------------------------------ */

/*
 * The allocated data has the following general form:
 *
 *  ccmallocKeepFreeData ?		// only if keep_deallocated_data and
 *					// and not only_count
 *
 *  ccmallocLinks ?			// if not only_count
 *
 *  ccmallocSize
 *
 *  ccmallocMagicAndFlags		// for all allocated data
 *
 *  ccmallocBoundary ?			// for detection of underwrites
 *
 *  ccmallocUserData
 *
 *  ccmallocBoundary ?			// for detection of overwrites
 *
 */
/* ------------------------------------------------------------------------ */

typedef struct CallChain_ * CallChain;

/* ------------------------------------------------------------------------ */

typedef struct ccmallocKeepFreeData
{
  CallChain freed_at;
}
ccmallocKeepFreeData;

/* ------------------------------------------------------------------------ */

typedef struct ccmallocLinks
{
  CallChain callchain;		/* of allocator if allocated and of
                                 * deallocator if deallocated
				 */

  struct ccmallocLinks * next, * prev;

  /* doubly linked list of data allocated from
   * same callchain (the callchain above!)
   */

  /*
   * We could use a singly linked list if keep_deallocated_data is true
   * and share the prev field with the freed_at pointer. But this would
   * only save a pointer when keep_deallocated_data is true in which case
   * we waste space anyway.
   */
}
ccmallocLinks;

/* ------------------------------------------------------------------------ */

typedef struct ccmallocSize
{
  int size;			/* If we had an own `malloc' this space
                                 * could be saved (the underlying allocator
				 * must store this value anyway)
				 */
}
ccmallocSize;

/* ------------------------------------------------------------------------ */

typedef struct ccmallocMagicAndFlags
{
  unsigned int magic_and_flags;	/* We use some `magic' here to detect
				 * if the user has messed up our data
				 * (see above). It would be better to use
				 * something like mprotect. But then we must
				 * store this data apart from the user data
				 */
}
ccmallocMagicAndFlags;

/* ------------------------------------------------------------------------ */

/* !!! just a comment !!! */

/* #define ccmallocUserData char */

/* ------------------------------------------------------------------------ */

typedef struct ccmallocBoundary
{
  unsigned boundary[0];
}
ccmallocBoundary;

/* ------------------------------------------------------------------------ */

typedef struct CallChainKey
{
  CallChain next;
  caddr_t addr;
}
CallChainKey;

/* ------------------------------------------------------------------------ */

enum AddrDataState { NOT_WRITTEN, WRITTEN, FIELDS_ARE_VALID };

/* ------------------------------------------------------------------------ */

typedef struct AddrData
{
  caddr_t key;

  enum AddrDataState state;

  char * name;			/* The name of the function. It can be
				 * found for (at least) all external
				 * functions if the binary is not stripped
				 */

  char * file;			/* Only binaries compiled with */
  int lineno;			/* `-g' have this info ! */

  char * alternative_name;	/* as provided by gdb */
}
AddrData;

/* ------------------------------------------------------------------------ */

typedef struct CallChainStats
{
  long bytes_allocated, bytes_deallocated, num_allocated, num_deallocated;
}
CallChainStats;

/* ------------------------------------------------------------------------ */

typedef struct CCToplevelData
{
  ccmallocLinks * allocations;	/* doubly linked list of data allocated */
  long bytes_wasted;		/* used for sorting */
  struct CCToplevelData * prev, * next;
  CallChainStats * stats;
}
CCToplevelData;

/* ------------------------------------------------------------------------ */

typedef struct CallChain_
{
  CallChainKey key;
  AddrData * addr_data;
  CCToplevelData * toplevel_data;
}
CallChain_;

/* ------------------------------------------------------------------------ */

enum TypeOfLogFile { NO_COMPRESSED_LOGFILE, COMPRESS_LOGFILE, GZIP_LOGFILE };

/* ------------------------------------------------------------------------ */

#define MAXBUFFER 1000
static char buffer[MAXBUFFER];		/* used by several parsers below */

/* ------------------------------------------------------------------------ */
/* file names
 */

static char * file_name = 0;				/* of executable */
static const char * startup_file_name = ".ccmalloc";
static int startup_file_lineno = 0;

/* Call chains that are not supposed to be logged.
 */

typedef struct FuncOrFile
{
  char *name; /* File or function name. */
  int line;   /* Optional line number in file or function. -1 = ignore. */
}
FuncOrFile;

typedef struct DontLogChain
{
  int num;                    /* Number of FuncOrFiles. */
  struct FuncOrFile funcs[1]; /* Actually more than this! See numelem! */
} 
DontLogChain;

static DontLogChain**dont_log_chains = 0; /* Chains that shouldn't be logged.*/
static int num_dont_log_chains = 0; /* How many of them there are. */
static DontLogChain**only_log_chains = 0; /* Chains that should be logged.*/
static int num_only_log_chains = 0; /* How many of them there are. */

/* ------------------------------------------------------------------------ */
/* global boolean flags accessible by startup file commands
 */

static int align_on_8_byte_boundary = 0;
static int callchain_statistics = 0;
static int chain_length = 0;
static int check_free_space = 0;
static int check_interval=0;
static int check_overwrites = 0;
static int check_start=0;
static int check_underwrites = 0;
static int continue_flag = 0;
static int empty_lines = 1;
static int file_info = 1;
static int keep_deallocated_data = 0;
static int library_chains = 0;
static int load_dynlibs_with_gdb = 0;
static int only_count = 0;
static int print_addresses_flag = 0;
static int print_on_one_line = 0;
static int silent_startup = 0;
static int sort_by_size = 1;
static int sort_by_wasted = 1;

#ifdef DEBUG
static int debug_flag = 1;
#else
static int debug_flag = 0;
#endif

static int check_modulo = 0;
static long check_count = 0;
static long checks_done = 0;

static enum TypeOfLogFile compressed_logfile = NO_COMPRESSED_LOGFILE;
static FILE * logfile = stderr;
static char * logfilename = 0;

/* ------------------------------------------------------------------------ */
/* statistics
 */

static long bytes_allocated = 0, num_allocated = 0, 
            bytes_deallocated = 0, num_deallocated = 0,
	    bytes_wasted;

/* ------------------------------------------------------------------------ */
/* internal flags
 */

static int ccmalloc_report_flag = 0;

#ifdef USE_BIDIRECTIONAL_CODE
static int show_address_while_reading_from_gdb = 0;
#endif

/* ------------------------------------------------------------------------ */
/* sort of inheritance support ;-)
 */

/* for data header ... initialized by ccmalloc_init() */

static int offset = -1;	/* depends on global flags only_count and
                         * keep_deallocated_data which can be set
			 * by the user
			 */

static int ccmallocKeepFreeData_ccmallocLinks_offset = -1;
static int ccmallocLinks_ccmallocMagicAndFlags_offset = -1;
static int ccmallocLinks_ccmallocUserData_offset = -1;
static int ccmallocLinks_ccmallocSize_offset = -1;
static int ccmallocMagicAndFlags_ccmallocUserData_offset = -1;
static int ccmallocSize_ccmallocMagicAndFlags_offset = -1;
static int ccmallocSize_ccmallocUserData_offset = -1;

#define is_allocated_links(links) \
  is_allocated((ccmallocMagicAndFlags*) \
    (((char*)links) + ccmallocLinks_ccmallocMagicAndFlags_offset))

#define is_corrupted_links(links) \
  is_corrupted((ccmallocMagicAndFlags*) \
    (((char*)links) + ccmallocLinks_ccmallocMagicAndFlags_offset))

#define get_size_links(links) \
  (((ccmallocSize*) \
    (((char*)links) + ccmallocLinks_ccmallocSize_offset)) -> size)

/* ------------------------------------------------------------------------ */

/* internal state of this library
 */

enum State
{
  UNINITIALIZED,
  INITIALIZING,
  INITIALIZED,
  TRIED_TO_REPORT_ONCE,		/* only used ifdef USE_DTOR_FOR_REPORTING */
  FINISHED
};

static enum State state = UNINITIALIZED;
static int tell_about_dtors = 1;

/* ------------------------------------------------------------------------ */

static HashTable strings = 0;
static HashTable addrs = 0;

static int num_symbols = 0;	
static Symbol ** symbols = 0;	/* sorted by address (zero terminated)  */
static HashTable symtab = 0;	/* fast access through name */

static int num_chains = 0;
static CallChain * chains;	/* zero terminated array of size num_chains */
static HashTable chaintab = 0;

static CallChain empty_call_chain = 0;

static CCToplevelData * toplevels = 0;

/* ------------------------------------------------------------------------ */

#if defined(OS_IS_SUNOS) || defined(OS_IS_SOLARIS)
/* `double' has to be 8 Byte aligned in these OS. This means we always have
 * to align on a 8 Byte boundary :-(
 */
# define ALIGN_MASK 7
#else
# define ALIGN_MASK (align_on_8_byte_boundary ? 7 : 3)
#endif

/* ------------------------------------------------------------------------ */

#define isAligned(c) ((((unsigned)c) & ALIGN_MASK) == 0)

#define align(c)					\
{							\
  if(!isAligned(c))					\
    {							\
      *(unsigned*)&c = (((unsigned)c) | ALIGN_MASK);	\
      *(unsigned*)&c = (((unsigned)c) + 1);		\
    }							\
}

/* ------------------------------------------------------------------------ */
/* macros for manipulation of `magic_and_flags'
 */

/* used for randomization of hash value */

#define MAGIC_CHAR ((char)0x42)
#define BASIC_SPELL 0x42424242
#define BASIC_SPELL_STR "0x42424242"

#define IS_ALLOCATED_FLAG	1
#define IS_CORRUPTED_FLAG	2

#define NUMBER_OF_FLAGS		2

#define FLAGS_MASK (IS_ALLOCATED_FLAG | IS_CORRUPTED_FLAG)
#define MAGIC_MASK (~FLAGS_MASK) 

#define set_bits(a,bits,mask) \
  ((a) -> magic_and_flags = \
    ((((a) -> magic_and_flags) & (~(mask))) | ((bits) & (mask))))

#define get_bits(a,mask) ((a) -> magic_and_flags & (mask))

/* Before you call `set_magic(p)' you should set the bits below. This also
 * means that after changing one of these bits you have to call `set_magic'
 * again. The same is true for changing the `size' field.
 */

#define set_is_allocated(a) set_bits((a),IS_ALLOCATED_FLAG,IS_ALLOCATED_FLAG)
#define clr_is_allocated(a) set_bits((a),0,IS_ALLOCATED_FLAG)
#define is_allocated(a) get_bits((a),IS_ALLOCATED_FLAG)

#define set_is_corrupted(a) set_bits((a),IS_CORRUPTED_FLAG,IS_CORRUPTED_FLAG)
#define clr_is_corrupted(a) set_bits((a),0,IS_CORRUPTED_FLAG)
#define is_corrupted(a) get_bits((a),IS_CORRUPTED_FLAG)

#define get_size(a) \
  (((ccmallocSize*)(((char*)a) - ccmallocSize_ccmallocMagicAndFlags_offset)) \
    -> size)

/* This macro hashes the `flags', the address and
 * the `size' of the argument into an unsigned number.
 * This can be used to detect manipulation of our data by the user.
 */

#define calculate_spell(a) \
(                                                                   \
  (                                                                 \
    ((unsigned int)(a))                          /* hash address */ \
    ^                                                               \
    (get_bits(a,FLAGS_MASK) << NUMBER_OF_FLAGS)  /* hash flags */   \
    ^                                                               \
    (get_size(a) << NUMBER_OF_FLAGS)             /* hash size */    \
    ^                                                               \
    BASIC_SPELL                                  /* randomize */    \
  )                                                                 \
  &                                                                 \
  MAGIC_MASK                                                        \
)

#define set_magic(a) set_bits((a), calculate_spell(a), MAGIC_MASK)

#define invalidate_magic(a) set_bits((a), ~calculate_spell(a), MAGIC_MASK)

#define is_valid_magic(a) (get_bits((a), MAGIC_MASK) == calculate_spell((a)))

/* ------------------------------------------------------------------------ */

static void _callchain_msg(const char * fmt, va_list ap)
{
  vfprintf(logfile, fmt, ap);
  fflush(logfile);
}

/* ------------------------------------------------------------------------ */

static void callchain_msg(const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  _callchain_msg(fmt, ap);
  va_end(ap);
}

/* ------------------------------------------------------------------------ */

static void close_logfile();

static void die()
{
  fputs("*** good bye cruel world ...\n", stderr);
  fflush(stderr);

  close_logfile();

  kill(getpid(), SIGSEGV);
  exit(1);
}

/* ------------------------------------------------------------------------ */

/* fatal error in this library
 */

static void callchain_fatal(const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  _callchain_msg(fmt,ap);
  va_end(ap);
  die();
}

/* ------------------------------------------------------------------------ */

/* normal error in this library
 */

static void callchain_error(const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  _callchain_msg(fmt,ap);
  va_end(ap);
  
  close_logfile();

  exit(1);
}

/* ------------------------------------------------------------------------ */

/* `readLine' reads a line from a file into a buffer with size `max' with
 * '\0' and '\n' as line seperators.
 */

static int readLine(FILE * file, char * buffer, int max)
{
  if(feof(file))
    {
      buffer[0] = '\0';
      return 0;
    }
  else
    {
      char ch;
      int i = 0;

      for(;;)
        {
          ch = getc(file);

	  if(ch == EOF || ch == '\0' || ch == '\n')
	    {
      	      buffer[i] = '\0';
      
      	      if(i==0) return ch == '\n' || ch == '\0';
      	      else return 1;
            }
	  else
            {
      	      if(i < max - 1)
      	        {
		  if(ch != '\n' && ch != '\0') buffer[i++] = ch;
      	        }
      	      else
      	        {
      	          callchain_error(
		    "buffer exceeded in readLine()\n", logfile);
      	        }
	    }
        }
    }
}

/* ------------------------------------------------------------------------ */
/* support functions for strings unique table */

static void freeString(void * p) { free(p); }

/* ------------------------------------------------------------------------ */

#define cmpStrings ((int(*)(void*,void*)) strcmp)

/* ------------------------------------------------------------------------ */

static char * find_string(char * s)
{
  void ** position;
  char * res;

  if(!strings)
    strings = new_HashTable(
      100, cmpStrings, hashpjw_HashTable, 0, freeString, 0);
  
  position = get_position_of_key_in_HashTable(strings, s);
  if(!*position)
    {
      res = strcpy((char*)malloc(strlen(s)+1), s);
      insert_at_position_into_HashTable(strings, position, res);
    }
  else
    {
      res = (char*) get_data_from_position_in_HashTable(strings, position);
    }

  return res;
}

/* ------------------------------------------------------------------------ */

static AddrData * new_AddrData(caddr_t key)
{
  AddrData * ad = (AddrData *) malloc(sizeof(AddrData));

  ad -> key = key;
  ad -> state = NOT_WRITTEN;
  ad -> name = 0;
  ad -> file = 0;
  ad -> lineno = -1;
  ad -> alternative_name = 0;
  
  return ad;
}

/* ------------------------------------------------------------------------ */

static int cmpAddr(void * k1, void * k2)
{
  AddrData * ad1 = (AddrData *) k1, * ad2 = (AddrData *) k2;
  caddr_t addr1 = ad1 -> key, addr2 = ad2 -> key;

  if(addr1 == addr2) return 0;
  else if(addr1 < addr2) return -1;
  else return 1;
}

/* ------------------------------------------------------------------------ */

static void * getKeyOfAddr(void * d)
{
  AddrData * ad = (AddrData *) d;
  return (void*) &ad -> key;
}

/* ------------------------------------------------------------------------ */

static void freeAddr(void * p) { free(p); }

/* ------------------------------------------------------------------------ */

static int hashAddr(void * p)
{
  caddr_t * ad = (caddr_t *) p;
  return (int) *ad;
}

/* ------------------------------------------------------------------------ */

static AddrData * find_addr(caddr_t key)
{
  void ** position;
  AddrData * res;

  if(!key) return 0;
  
  if(!addrs)
    addrs = new_HashTable(100, cmpAddr, hashAddr, getKeyOfAddr, freeAddr, 0);
  
  position = get_position_of_key_in_HashTable(addrs, &key);
  if(!*position)
    {
      res = new_AddrData(key);
      insert_at_position_into_HashTable(addrs, position, res);
    }
  else
    {
      res = (AddrData*) get_data_from_position_in_HashTable(addrs, position);
    }
  
  return res;
}

/* ------------------------------------------------------------------------ */

static void freeAllocations(ccmallocLinks *);

/* ------------------------------------------------------------------------ */

static CCToplevelData * new_CCToplevelData()
{
  CCToplevelData * res;

  res = (CCToplevelData*) malloc(sizeof(CCToplevelData));

  res -> allocations = 0;
  res -> bytes_wasted = 0;

  if(callchain_statistics)
    {
      res -> stats = (CallChainStats*) malloc(sizeof(CallChainStats));
      res -> stats -> num_allocated = 0;
      res -> stats -> num_deallocated = 0;
      res -> stats -> bytes_allocated = 0;
      res -> stats -> bytes_deallocated = 0;
    }
  else res -> stats = 0;	/* defensive */

  return res;
}

/* ------------------------------------------------------------------------ */

static void free_CCToplevelData(CCToplevelData * td)
{
  if(callchain_statistics)
    {
      assert(td -> stats);
      free(td -> stats);
    }
  
  if(td -> allocations) freeAllocations(td -> allocations);
  
  free(td);
}

/* ------------------------------------------------------------------------ */
/* manipulation of doubly linked list `allocations' in a CallChain_
 */

static void enqueue(CallChain c, ccmallocLinks * d)
{
  CCToplevelData * td;

  if(!(td = c -> toplevel_data))
    {
      td = c -> toplevel_data = new_CCToplevelData();
      assert(td -> allocations == 0);
    }

  if(!td -> allocations)
    {
      /* enqueue td to toplevels */

      td -> next = toplevels;
      if(td -> next) td -> next -> prev = td;
      td -> prev = 0;

      toplevels = td;
    }

  d -> next = td -> allocations;
  if(d -> next) d -> next -> prev = d;
  d -> prev = 0;

  td -> allocations = d;
  d -> callchain = c;
}

/* ------------------------------------------------------------------------ */

static void dequeue(ccmallocLinks * d)
{
  CCToplevelData * td;

  CallChain c = d -> callchain;
  td = c -> toplevel_data;

  assert(td -> allocations);

  if(d -> prev) d -> prev -> next = d -> next;
  else
    {
      assert(td -> allocations == d);

      if(!(td -> allocations = d -> next))
        {
          /* and dequeue td from toplevels if no more allocations */

	  if(td -> prev) td -> prev -> next = td -> next;
	  else
	    {
	      assert(td == toplevels);

	      toplevels = td -> next;
	    }

	  if(td -> next) td -> next -> prev = td -> prev;
	}
    }
  
  if(d -> next) d -> next -> prev = d -> prev;
}

/* ------------------------------------------------------------------------ */

/* deallocated a list of ccmallocLinks 
 */

static void freeAllocations(ccmallocLinks * a)
{
  while(a)
    {
      ccmallocLinks * tmp = a -> next;
      char * b = ((char*)a);

      if(keep_deallocated_data)
        b -= ccmallocKeepFreeData_ccmallocLinks_offset;
      free(b);

      a = tmp;
    }
}


/* ------------------------------------------------------------------------ */
/* functions for `chaintab' hashtable
 */

/* compare first by `next' and than by `addr' */

static int cmpKeys(void * _a, void * _b)
{
  CallChainKey * a = (CallChainKey*)_a,
               * b = (CallChainKey*)_b;

  if(a != b)
    {
      if(a -> next == b -> next)
        {
	  if(a -> addr == b -> addr) return 0;
	  else if(a -> addr < b -> addr) return -1;
	  else return 1;
	}
      else
        {
	  if(a -> next < b -> next) return -1;
	  else return 1;
	}
    }
  else return 0;
}

/* ------------------------------------------------------------------------ */

static int hashKey(void * _a)
{
  CallChainKey * a = (CallChainKey*)_a;
  return ((int)a -> next) ^ ((int)a -> addr);
}

/* ------------------------------------------------------------------------ */

static void * getKey(void * a)
{
  return (void*) &((CallChain)a)->key;
}

/* ------------------------------------------------------------------------ */

static void _freeCallChain(CallChain c)
{
  if(c -> toplevel_data) free_CCToplevelData(c -> toplevel_data);
  free(c);
}

/* wrapper */

#define freeCallChain ((void(*)(void*)) _freeCallChain)

/* ------------------------------------------------------------------------ */

/* We do not delete CallChains until exit of the program.
 * So there are no garbage CallChains at all and we do not
 * need garbage collection (for example with reference
 * counting). All allocated CallChain_'s are stored in
 * the hashtable and can be reclaimed from there.
 */

static CallChain new_CallChain(CallChain next, caddr_t addr)
{
  CallChainKey key;
  CallChain res;
  void ** position;

  key.next = next;
  key.addr = addr;

  if(!chaintab)
    chaintab = new_HashTable(100, cmpKeys, hashKey, getKey, freeCallChain, 0);

  position = get_position_of_key_in_HashTable(chaintab, &key);

  if(*position)
    {
      res = (CallChain)
        get_data_from_position_in_HashTable(chaintab, position);
    }
  else
    {
      res = (CallChain) malloc(sizeof(CallChain_));
      insert_at_position_into_HashTable(chaintab, position, res);

      /* and initialized CallChain */

      res -> key = key;
      res -> addr_data = 0;
      res -> toplevel_data = 0;
    }

  return res;
}

/* ------------------------------------------------------------------------ */

/* `backtrace' returns the CallChain of frames above the current
 * and skips an initial amount of frames.
 *
 * The idea for this very important part of code
 * is inspired by the `mpr' memory profiler of
 * taj@intergate.bc.ca (Taj Khattra).
 */

static int have_bounds = 0;

#define is_entry_pc(c) \
  (have_bounds && ((c) < entry_end && (c) >= entry_start))

#define is_library_pc(c) \
  (have_bounds && ((c) < text_start || (c) >= text_end))

static caddr_t entry_start = 0;
static caddr_t entry_end = 0;

static caddr_t text_start = 0;
static caddr_t text_end = 0;

static int exception = 0;
static jmp_buf backtrace_jump;

static void (*old_SEGV_handler)(int) = 0;
static void (*old_BUS_handler)(int) = 0;

static CallChain bt_res = 0;		/* save for longjmp's */

/* ------------------------------------------------------------------------ */

static void set_exception(int i)
{
  (void) i;
  exception = 1;
  longjmp(backtrace_jump, 1);
}

/* ------------------------------------------------------------------------ */

static void setup_SEGV_BUS_handlers()
{
  exception = 0;
  old_SEGV_handler = signal(SIGSEGV, set_exception);
  old_BUS_handler = signal(SIGBUS, set_exception);
}

/* ------------------------------------------------------------------------ */

static void restore_SEGV_BUS_handlers()
{
  (void) signal(SIGBUS, old_BUS_handler);
  (void) signal(SIGSEGV, old_SEGV_handler);
}

/* ------------------------------------------------------------------------ */
#ifdef X86_BACKTRACE
/* ------------------------------------------------------------------------ */

/* special code for x86 (or just linux ?)
 */

static CallChain backtrace(int skip)
{
  bt_res = 0;
  assert(skip >= 0);

  if(chain_length == 1) return 0;

  setup_SEGV_BUS_handlers();

  if(setjmp(backtrace_jump) == 0)
    {
      int i = 0, max = skip + chain_length;
      caddr_t pc, * fp = ((caddr_t*) & skip) - 2;	/* x86 */
      exception = 1;

      while(1)
        {
	  /* A simple break means always:
	   * abort this loop with an exception (=1)
	   */

	  caddr_t * new_fp;

	  /* find the frame above fp and check it */

          if(!fp) break;
          new_fp = *(caddr_t **) fp;		/* raise signal ?? */

	  /* extract program counter from old frame and check it */

          pc = ((caddr_t *) fp) [1];		/* raise signal ?? */
          if(!pc) break;			/* heuristic */
	  if(have_bounds && pc < text_start) break; /* libraries are above */

          if(chain_length>0 && i>=max) break;

          if(is_entry_pc(pc))
	    {
	      exception=0;	/* normal exit of this loop */
	      break;
	    }

          if(!new_fp) break;			/* heuristic */
	  if(new_fp <= fp) break;		/* stack grows downward */ 

          if(skip == 0) bt_res = new_CallChain(bt_res, pc);
          else skip--;

	  /* increment */

          i++;
	  fp = new_fp;
        }
    }
  
  restore_SEGV_BUS_handlers();

  if(exception)
    {
      /* generate a non valid frame with address 0
       * above already generated call chain
       */

      bt_res = new_CallChain(bt_res, 0);	/* is zero ok ? */
    }

  return bt_res;
}

#else

/* ------------------------------------------------------------------------ */

/* This should work with gcc on all platforms but has the drawback
 * of being a little bit slower and of course works only for fixed
 * (at compile time) maximal length of callchains.
 * (because __builtin_return_address expects a constant as argument).
 */

#define MAXCALLCHAINLENGTH 100
#define RA(a) case a: return (caddr_t) __builtin_return_address(a);

static caddr_t return_address(unsigned i)
{
  switch(i)
    {
RA(0);RA(1);RA(2);RA(3);RA(4);RA(5);RA(6);RA(7);RA(8);RA(9);RA(10);
RA(11);RA(12);RA(13);RA(14);RA(15);RA(16);RA(17);RA(18);RA(19);RA(20);
RA(21);RA(22);RA(23);RA(24);RA(25);RA(26);RA(27);RA(28);RA(29);RA(30);
RA(31);RA(32);RA(33);RA(34);RA(35);RA(36);RA(37);RA(38);RA(39);RA(40);
RA(41);RA(42);RA(43);RA(44);RA(45);RA(46);RA(47);RA(48);RA(49);RA(50);
RA(51);RA(52);RA(53);RA(54);RA(55);RA(56);RA(57);RA(58);RA(59);RA(60);
RA(61);RA(62);RA(63);RA(64);RA(65);RA(66);RA(67);RA(68);RA(69);RA(70);
RA(71);RA(72);RA(73);RA(74);RA(75);RA(76);RA(77);RA(78);RA(79);RA(80);
RA(81);RA(82);RA(83);RA(84);RA(85);RA(86);RA(87);RA(88);RA(89);RA(90);
RA(91);RA(92);RA(93);RA(94);RA(95);RA(96);RA(97);RA(98);RA(99);RA(100);
      default: return 0;
    }
}

static CallChain backtrace(int skip)
{
  bt_res = 0;
  assert(skip >= 0);

  setup_SEGV_BUS_handlers();

  if(chain_length == 1) return 0;

  if(setjmp(backtrace_jump) == 0)
    {
      int i = ++skip, max = skip + chain_length;
      exception = 1;

      while(1)
        {
	  /* a simple break means always abort this loop
	   * with exception == 1
	   */

          caddr_t pc = return_address(i++);

	  if(!pc) break;
	  if(have_bounds && pc < text_start) break; /* libraries are above */

          if(i>MAXCALLCHAINLENGTH || (chain_length>0 && i>=max)) break;

          if(is_entry_pc(pc))
	    {
	      exception = 0;	/* normal exit of this loop */
	      break;
	    }

          bt_res = new_CallChain(bt_res, pc);
        }
    }
  
  restore_SEGV_BUS_handlers();

  if(exception)
    {
      /* generate a non valid frame with address 0
       * above already generated call chain
       */

      bt_res = new_CallChain(bt_res, 0);	/* is zero ok ? */
    }

  return bt_res;
}

#endif

/* ------------------------------------------------------------------------ */

/* `has_allocated_data' tells us if a `CallChain' has some allocated but not
 * deallocated `ccmallocLinks' associated with it. Depending on the value
 * of `callchain_statistics' and `keep_deallocated_data' the semantic
 * changes!
 */

static int has_allocated_data(CallChain c)
{
  CCToplevelData * td = c -> toplevel_data;

  assert(c);

  if(!td || !td -> allocations) return 0;

  if(callchain_statistics)
    {
      CallChainStats * stats = td -> stats;
      return stats -> num_deallocated || stats -> num_allocated;
    }
  else
    {
      if(keep_deallocated_data)
        {
          int found_allocated_data = 0;
          ccmallocLinks * p;

          for(p=td->allocations; p; p=p->next)
            {
      	      if(is_allocated_links(p))
      	        {
      	          found_allocated_data = 1;
      	          break;
      	        }
            }
          
          return found_allocated_data;
        }
      else
        {
	  /* In this case we have only garbage on the `allocations' list.
	   */

	  return td -> allocations != 0;	/* defensive (== return 1) */
        }
    }
}

/* ------------------------------------------------------------------------ */

/* `name_all_chains' assumes that the symbol array `symbols' is installed
 * and sorted. Then it trys to find a symbol name for all addresses
 * stored in callchains. The search method is a binary search.
 */

static Symbol * find_symbol_by_address(caddr_t addr)
{
  if(is_library_pc(addr)) return 0;

  if(num_symbols)
    {
      int l = 0, h = num_symbols - 1;

      while(h - l > 1)			/* binary search */
        {
          int m = (l+h)/2;
          caddr_t maddr = symbols[m] -> addr;

          if(maddr > addr) h = m;
          else
          if(maddr < addr) l = m;
          else l = h = m;		/* does this really happen ? */
        }				/* (first opcode of a function */
  				   	/*  should be no `call') */
      if(h > l)
        {
          if(symbols[h] -> addr <= addr) l = h;
          else h = l;				/* defensive */
        }
      
      return symbols[l];
    }
  else return 0;
}

/* ------------------------------------------------------------------------ */

static void name_chain(CallChain c)
{
  while(c)
    {
      caddr_t addr = c -> key.addr;

      if(addr)
        {
	  AddrData * ad;
	  if(!c -> addr_data) c -> addr_data = find_addr(addr);
	  ad = c -> addr_data;
	  if(!ad -> name)
	    {
              Symbol * s = find_symbol_by_address(addr);
              if(s) ad -> name = s -> name;
	    }
        }
      
      c = c -> key.next;
    }
}

/* ------------------------------------------------------------------------ */

static void name_all_chains()
{
  int i;

  fputs("| retrieving function names for addresses ...", logfile);
  fflush(logfile);

  for(i=0; i<num_chains; i++)
    name_chain(chains[i]);

  fputs(" done.   |\n", logfile);
  fflush(logfile);
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/* This part of the code is very unix like:
 * We fork a child that executes gdb and sends us information about the
 * file and line number of addresses.
 */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

/* parse an answer from gdb to a `info line' command and insert this info
 * into the given callchain if the answer is valid.
 */

static void parseGdbInfoLineAnswer(CallChain c, char * buffer)
{
  AddrData * addr_data = c -> addr_data;
  assert(addr_data);

  assert(addr_data -> state != FIELDS_ARE_VALID);

  if(!(buffer = strtok(buffer, " "))) return;

  if(strcmp(buffer, "No") == 0) return;		/* "No symbol information" */
  if(strcmp(buffer, "Line") != 0) return;	/* be silent otherwise */

  if(!(buffer = strtok(0, " "))) return;	/* search for line number */

  addr_data -> lineno = atoi(buffer);

  /* skip characters until `"' */

  if(!(buffer = strtok(0, "\""))) return;
  if(!(buffer = strtok(0, "\""))) return;	/* search for file name */

  addr_data -> file = find_string(buffer);

  /* should test range and function name too ... */
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

/* parse a string containing a demangled name with the syntax:
 *
 *        ^ [^'<']* '<' demangled_name '+' .* '>' .* $
 *
 * and insert 
 */

static void parseGdbDemangledInfo(CallChain c, char * buffer)
{
  AddrData * addr_data = c -> addr_data;
  assert(addr_data);

  if(addr_data -> alternative_name) return;

  if(!(buffer = strtok(buffer, "<"))) return;
  if(!(buffer = strtok(0, "+>"))) return;

  if(!addr_data -> name ||
     strcmp(addr_data -> name, buffer) != 0)
    {
      assert(!addr_data -> alternative_name); /* else garbage */
      addr_data -> alternative_name = find_string(buffer);
    }
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

/* ------------------------------------------------------------------------ */
#ifdef USE_BIDIRECTIONAL_CODE
/* ------------------------------------------------------------------------ */

/* Use a bidirectional pipe (write and read) to get file information from
 * gdb. This was very slow and did not work very well under SunOS because
 * sometimes reading from gdb was stalled. I want to keep this code so I
 * do not delete from distribution. Perhaps someone knows why gdb stalled.
 */

static int ends_with_gdb_prompt(char * buffer)
{
  char * last = 0;

  while(*buffer)
    {
      while(*buffer && *buffer != '(')
        buffer++;
      
      if(*buffer) last = buffer++;
    }
  
  return last && strncmp(last, "(gdb)", 5) == 0;
}

/* ------------------------------------------------------------------------ */

static int gdb_broken_pipe = 0;	/* for child <-> parent communication
                                 * through the signal SIGPIPE
				 * (see gdb_pipe_error_handler)
				 */

/* ------------------------------------------------------------------------ */

static void gdb_pipe_error_handler(int sig)
{ 
  (void)sig;			/* no warnings with C++ */
  gdb_broken_pipe = 1;
}

/* ------------------------------------------------------------------------ */
#ifdef DEBUG_GDB_PIPE
/* ------------------------------------------------------------------------ */

/* Immediate log to stderr or to the log file did not work. When I did this
 * the read_from_gdb and read_gdb_prompt got stalled. Now I log to memory
 * and dump all when we finished.
 */

static char * debug_gdb_pipe_buffer = 0;
static int debug_gdb_pipe_buffer_pos = 0;
static int debug_gdb_pipe_buffer_sz = 0;

/* ------------------------------------------------------------------------ */

static void debug_gdb_pipe_buffer_write(const char * str, int new_line)
{
  int l = strlen(str);

  new_line = new_line ? 1 : 0;

  if(!debug_gdb_pipe_buffer)
    {
      debug_gdb_pipe_buffer_sz = 10000;
      debug_gdb_pipe_buffer =
        (char*) malloc(debug_gdb_pipe_buffer_sz);
      debug_gdb_pipe_buffer_pos = 0;
    }

  if(debug_gdb_pipe_buffer_pos + l + new_line >= debug_gdb_pipe_buffer_sz)
    {
      char * new_buffer;

      do
        {
          debug_gdb_pipe_buffer_sz *= 2; 
        }
      while(debug_gdb_pipe_buffer_pos + l + new_line
            >=
            debug_gdb_pipe_buffer_sz);
      
      new_buffer = (char*) malloc(debug_gdb_pipe_buffer_sz);
      strcpy(new_buffer, debug_gdb_pipe_buffer);
      free(debug_gdb_pipe_buffer);
      debug_gdb_pipe_buffer = new_buffer;
    }
  
  sprintf(debug_gdb_pipe_buffer + debug_gdb_pipe_buffer_pos,
          "%s", str);

  debug_gdb_pipe_buffer_pos += l;

  if(new_line)
    {
      *(debug_gdb_pipe_buffer + debug_gdb_pipe_buffer_pos) = '\n';
      debug_gdb_pipe_buffer_pos++;
      *(debug_gdb_pipe_buffer + debug_gdb_pipe_buffer_pos) = '\0';
    }
}

/* ------------------------------------------------------------------------ */

static void debug_gdb_pipe_buffer_report()
{
  callchain_msg("%s\n", debug_gdb_pipe_buffer);
}

/* ------------------------------------------------------------------------ */

static void strip_new_line(char * s)
{
  while(*s && *s !='\n') s++;
  if(*s) *s='\0';
}

/* ------------------------------------------------------------------------ */
#endif
/* ------------------------------------------------------------------------ */

static void (*gdb_old_handler)(int);
static int gdb_in[2], gdb_out[2];
static int gdb_child, found_gdb_prompt = 0, gdb_started = 0;

/* ------------------------------------------------------------------------ */

static void shut_down_gdb()
{
  if(file_info && gdb_started)
    {
      gdb_started = 0;

      close(gdb_in[0]);
      close(gdb_out[1]);
  
      if(gdb_child != -1)
        {
          kill(gdb_child, SIGTERM);
          wait(0);
        }
      else
        {
          close(gdb_in[1]);	/* otherwise closed by `init_gdb_parent' */
          close(gdb_out[0]);
        }
  
      signal(SIGPIPE, gdb_old_handler);

      found_gdb_prompt = 0;	/* defensive */
    }
  
# ifdef DEBUG_GDB_PIPE
    {
      debug_gdb_pipe_buffer_report();
      debug_gdb_pipe_buffer_write("shut_down_gdb: pipe closed", 1);
      if(debug_gdb_pipe_buffer) free(debug_gdb_pipe_buffer);
    }
# endif
}

/* ------------------------------------------------------------------------ */
#if defined(GDB_PIPE_CHILD_INPUT_IS_NON_BLOCKED) \
    defined(GDB_PIPE_CHILD_OUTPUT_IS_NON_BLOCKED)
/* ------------------------------------------------------------------------ */

static void set_non_block(int fd)
{
  int fl;

  fl = fcntl(fd, F_GETFL, 0);
  fl |= O_NONBLOCK;
  fcntl(fd, F_SETFL, fl);
}

/* ------------------------------------------------------------------------ */
#endif
/* ------------------------------------------------------------------------ */

static void execute_gdb_child()
{
  close(gdb_in[1]);	/* only parent writes to this fd */
  close(gdb_out[0]);	/* only parent reads from this fd */

  close(0);		/* use gdb_in[0] as stdin */
  dup2(gdb_in[0], 0);
  close(gdb_in[0]);

  close(1);		/* use gdb_out[1] as stdout */
  dup2(gdb_out[1], 1);
  close(gdb_out[1]);

# ifdef GDB_PIPE_CHILD_OUTPUT_IS_NON_BLOCKED
    {
      set_non_block(1);
    }
# endif

  close(2);

  signal(SIGPIPE, gdb_old_handler);

  assert(file_name);

  execlp("gdb", "gdb", "-q", "-nx", file_name, 0);

  kill(getppid(), SIGPIPE);	/* -> gdb_broken_pipe == 1 in parent */
  exit(1);
}

/* ------------------------------------------------------------------------ */

static void init_gdb_parent()
{
  close(gdb_in[0]);	/* only child reads from this fd */
  close(gdb_out[1]);	/* only child writes to this fd */

# ifdef GDB_PIPE_CHILD_INPUT_IS_NON_BLOCKED
    {
      /* set input to child process to non blocking write mode */

      set_non_block(gdb_in[1]);
    }
# endif

  found_gdb_prompt = 0;
}

/* ------------------------------------------------------------------------ */

static void call_gdb()
{
  if(!file_info) return;

  gdb_started = 1;

  if(pipe(gdb_in) == -1 || pipe(gdb_out) == -1)
    {
      callchain_msg("could not generate pipes for reading `gdb'\n");
      return;
    }
  
  gdb_broken_pipe = 0;
  gdb_old_handler = signal(SIGPIPE, gdb_pipe_error_handler);

  switch((gdb_child = fork()))
    {
      case 0:				/* child */

        execute_gdb_child();		/* will never return */
        assert(0);
        exit(1);

      default:				/* parent */

        init_gdb_parent();
        break;				/* continue ... */

      case -1:				/* error */

        callchain_msg("could not fork child for reading from `gdb'\n");
        shut_down_gdb();
        return;
    }

# ifdef DEBUG_GDB_PIPE
    debug_gdb_pipe_buffer_write("call_gdb: pipe opened", 1);
# endif
}

/* ------------------------------------------------------------------------ */

/* Try to read gdb prompt. Assume the pipe to gdb has already been set up.
 * Use found_gdb_prompt to determine if the prompt has already beed read by
 * a previous read operation from the gdb pipe. Return 1 if an error occured.
 * In that case also shut down the gdb pipe. Otherwise return 0.
 */

static int read_gdb_prompt()
{
  if(!found_gdb_prompt)
    {
      int n;

      n = read(gdb_out[0], buffer, MAXBUFFER-1);

      if(gdb_broken_pipe || n < 0)
	{
	  callchain_msg("could not read gdb prompt\n");
	  shut_down_gdb();
	  return 1;
	}
      
      buffer[n]='\0';		/* that's why we used MAXBUFFER - 1 */

#     ifdef DEBUG_GDB_PIPE
	strip_new_line(buffer);
	debug_gdb_pipe_buffer_write("<1> from gdb: ", 0);
	debug_gdb_pipe_buffer_write(buffer, 1);
#     endif
    }
  
  found_gdb_prompt = 0;
  return 0;
}

/* ------------------------------------------------------------------------ */

/* Write args to gdb. Return 0 on success 1 on failure.
 */

static int write_to_gdb(const char * fmt, ...)
{
  int l, n;
  const char * p;

  va_list ap;
  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  for(p = buffer, l = strlen(buffer); *p; p += n, l -= n)
    {
#     ifdef DEBUG_GDB_PIPE
	strip_new_line(buffer);
	debug_gdb_pipe_buffer_write("<2> to   gdb: ", 0);
	debug_gdb_pipe_buffer_write(buffer, 1);
#     endif

      n = write(gdb_in[1], p, l);

      if(gdb_broken_pipe || (n <= 0 && errno != EAGAIN))
        {
	  callchain_msg("could not write to gdb\n");
	  shut_down_gdb();
	  return 1;
	}
    }
  
  return 0;
}

/* ------------------------------------------------------------------------ */

/* Reads data from gdb into the global buffer. Reads as much as it can and
 * sets `found_gdb_prompt'.
 */

static int read_from_gdb()
{
  int n;

  n = read(gdb_out[0], buffer, MAXBUFFER-1);

  if(gdb_broken_pipe || n < 0)
    {
      callchain_msg("could not open pipe to gdb\n");
      shut_down_gdb();
      found_gdb_prompt = 0;	/* defensive */
      return 1;
    }
  
  buffer[n] = '\0';

# ifdef DEBUG_GDB_PIPE
    strip_new_line(buffer);
    debug_gdb_pipe_buffer_write("<3> from gdb: ", 0);
    debug_gdb_pipe_buffer_write(buffer, 1);
# endif

  found_gdb_prompt = ends_with_gdb_prompt(buffer);
  return 0;
}

/* ------------------------------------------------------------------------ */

/* Insert info into the CallChain c that gdb can give us. That is the
 * filename, the line number, and a demangled name for each code address.
 */

static void insert_file_info_in_chain_from_bidirectional_pipe(CallChain c)
{
  if(!file_info) return;

  while(c)	/* over all childs of c */
    {
      /* if something happens to the pipe to gdb then simple leave this loop
       * which means that we do not provide any detailed info
       * for the sibblings of the callchain
       */

      if(c -> key.addr)
        {
          if(!c -> addr_data) c -> addr_data = find_addr(c -> key.addr);

          if(c -> addr_data -> state != FIELDS_ARE_VALID)
            {
              if(show_address_while_reading_from_gdb)
	        {
	          fputs("", logfile);
	          fprintf(logfile, "%08x", (unsigned int) c -> key.addr);
                  fflush(logfile);
	        }

              if(read_gdb_prompt() ||
                 write_to_gdb("info line *0x%08x\n", c -> key.addr) ||
	         read_from_gdb()) break;

              parseGdbInfoLineAnswer(c, buffer);

#             if !defined(NM_DEMANGLES_WELL)
                {
                  /* The info line command returns a *not* demangled name
	           * for the function of the code at `c -> key.addr' 8-(
	           * (GDB 4.15.1 at least ...)
	           */

	          if(read_from_gdb() ||
	             write_to_gdb("p (void(*)()) 0x%08x\n", c -> key.addr) ||
		     read_from_gdb()) break;
	      
	          parseGdbDemangledInfo(c, buffer);
	        }
#             endif
	      
	      c -> addr_data -> state = FIELDS_ARE_VALID;
	    }

	}

	c=c->key.next;
    }
}

/* ------------------------------------------------------------------------ */

static void insert_file_info_in_chains_from_bidirectional_pipe()
{
  CallChain * c;

# ifdef OS_IS_LINUX
    {
      if(logfile == stdout)
        show_address_while_reading_from_gdb=isatty(1);
      else
      if(logfile == stderr)
        show_address_while_reading_from_gdb=isatty(2);
      else
        show_address_while_reading_from_gdb = 0;
    }
# else
    show_address_while_reading_from_gdb = 0;
# endif

  if(show_address_while_reading_from_gdb) fputs("0x        ", logfile);
  else fputs("...", logfile);
  fflush(logfile);

  call_gdb();

  for(c=chains; *c; c++)
    insert_file_info_in_chain_from_bidirectional_pipe(*c);
  
  shut_down_gdb();

  if(show_address_while_reading_from_gdb)
    fputs("...", logfile);

  show_address_while_reading_from_gdb=0;
}

/* ------------------------------------------------------------------------ */
#else /* USE_BIDIRECTIONAL_CODE */
/* ------------------------------------------------------------------------ */

static void dump_chain(FILE * dump_file, CallChain c)
{
  while(c)
    {
      AddrData * ad = c -> addr_data;

      if(ad && ad -> state == NOT_WRITTEN)
        {
	  if(ad -> key == 0 || 
	     (!load_dynlibs_with_gdb && is_library_pc(ad -> key)))
	    {
	      ad -> state = FIELDS_ARE_VALID;
	      continue;
	    }
	  else
	    {
              fprintf(dump_file,
	              "info line * 0x%08x\n", (unsigned) ad -> key);

              ad -> state = WRITTEN;

#             ifdef DEBUG_GDB_PIPE
                callchain_msg(
	          "dump: info line * 0x%08x\n", (unsigned) ad -> key);
#	      endif

#if !defined(NM_DEMANGLES_WELL)
	      if ( 1 )
#else
	      if ( is_library_pc( c->key.addr ) )
#endif
	        {
	          fprintf(dump_file, "p (void(*)()) 0x%08x\n", 
	                  (unsigned) c -> key.addr);

#                 ifdef DEBUG_GDB_PIPE
	            callchain_msg("p (void(*)()) 0x%08x\n", ad -> key);
#                 endif
	        }
	    }
	}

      c = c -> key.next;
    }
}

/* ------------------------------------------------------------------------ */

static void dump_chains(FILE * dump_file)
{
  CallChain * c;
  for(c=chains; *c; c++)
    dump_chain(dump_file, *c);
}

/* ------------------------------------------------------------------------ */
/* return 1 on error */

static int undump_chain_info(FILE * gdb_pipe, CallChain c)
{
  while(c)
    {
      AddrData * ad = c -> addr_data;

      if(ad && ad -> state != FIELDS_ARE_VALID)
        {
	  int got_line = 0, no_line_line = 0, size = MAXBUFFER, offset = 0;

	  /* Why does gdb answer on several lines? So we have to merge
	   * several lines before we present it to parseGdbInfoLineAnswer.
	   * We got a full line (consisting of several lines read from
	   * the pipe) if it starts with "No line" or we found a dot at the
	   * end of a real line.
	   *
	   * Merging is done in the global buffer by increasing offset
	   * and writing at buffer + offset just past the previous read
	   * part of the line.
	   */

	  while(!got_line && size > 0)
	    {
	      if(readLine(gdb_pipe, buffer + offset, size))
	        {
		  int l = strlen(buffer + offset);

#		  ifdef DEBUG_GDB_PIPE
		    callchain_msg("undump: %s\n", buffer + offset);
#		  endif

		  if(strncmp(buffer, "No line", 7) == 0) no_line_line = 1;

		  if(( no_line_line && buffer[offset + l-1] == '>') ||
		     (!no_line_line && buffer[offset + l-1] == '.'))
		    got_line = 1;

		  if(got_line) no_line_line = 0;
		  else
		    {
		      size -= l;
		      offset += l;
		    }
	        }
	      else break;
	    }

	  if(got_line)
	    {
#	      ifdef DEBUG_GDB_PIPE
	        callchain_msg("UNDUMP: %s\n", buffer);
#	      endif

              parseGdbInfoLineAnswer(c, buffer);
	      ad -> state = FIELDS_ARE_VALID;
	    }
	  else
	    {
              fputs(" @$#!|&^              |\n", logfile);
	      callchain_msg("could not get all info from gdb\n");
	      return 1;
	    }

#if !defined(NM_DEMANGLES_WELL)
	  if ( 1 )
#else
	  if ( is_library_pc( c->key.addr ) )
#endif         
	    {
	      if(readLine(gdb_pipe, buffer, MAXBUFFER))
	        {
#		  ifdef DEBUG_GDB_PIPE
                    callchain_msg("UNDUMP: %s\n", buffer);
#		  endif

	          parseGdbDemangledInfo(c, buffer);
		}
	    }
	}
      
      c = c -> key.next;
    }
  
  return 0;
}

/* ------------------------------------------------------------------------ */
/* return 1 on error */

static int undump_chains_info(FILE * gdb_pipe)
{
  CallChain * c;
  for(c=chains; *c; c++)
    if(undump_chain_info(gdb_pipe, *c)) return 1;
  
  return 0;
}

/* ------------------------------------------------------------------------ */
/* If the argument is non zero then only the information for that CallChain
 * is generated. Otherwise line info is generated for all CallChains that
 * have allocated data.
 */

static int insert_file_info_in_chains_from_unidirectional_pipe(
  CallChain c)
{
  FILE * dump_file, * gdb_pipe;
  int error = 0;

  char tmp_file_name[200];
  char * cmd;

  sprintf(tmp_file_name, "/tmp/ccmalloc.dump.%d", (int) getpid());

  dump_file = fopen(tmp_file_name, "w");
  if(!dump_file)
    {
      fputs(" @$#!|&^              |\n", logfile);
      callchain_msg("could not write to `%s'\n", tmp_file_name);
      return 1;
    }

  if (load_dynlibs_with_gdb)
    /* It Works For Me (tm) (Johannes Keukelaar) */
    fprintf(dump_file, "break _init\nrun\n" );

# ifdef DEBUG_GDB_PIPE
    fputs("\n\n", logfile);
# endif
  
  if(c) dump_chain(dump_file, c);
  else dump_chains(dump_file);

  /* write a sentinel for checking that all went right
   */
  fprintf(dump_file, "p/x %s\n", BASIC_SPELL_STR);

# ifdef DEBUG_GDB_PIPE
    callchain_msg("dump: p/x %s\n", BASIC_SPELL_STR);
# endif

  fclose(dump_file);

  cmd = (char*) malloc(strlen(file_name) + 200);
  sprintf(cmd, "gdb -q -batch -x %s %s 2>/dev/null", tmp_file_name, file_name);

  gdb_pipe = popen(cmd, "r");

  if(load_dynlibs_with_gdb)
    {
      /* Read the output of the commands we inserted. */
      readLine(gdb_pipe, buffer, MAXBUFFER);
      readLine(gdb_pipe, buffer, MAXBUFFER);
      readLine(gdb_pipe, buffer, MAXBUFFER);
    }

  if(gdb_pipe)
    { 
      char * p;
      int error;

      if(c) error = undump_chain_info(gdb_pipe, c);
      else error = undump_chains_info(gdb_pipe);

      if(error)
        {
	  callchain_msg(
	    "oops, something went wrong while undumping file info\n");
          error = 1;
          goto SAVE_EXIT_00;
	}
      else
        {
          /* try to read sentinel that should be the following line
           * $1 = BASIC_SPELL_STR
           */

          if(!readLine(gdb_pipe, buffer, MAXBUFFER) ||
             !(p = strtok(buffer, " ")) ||			/* "$1" */
             !(p = strtok(0, " ")) ||			/* "=" */
             !(p = strtok(0, " ")) ||			/* BASIC_SPELL_STR */
             strncmp(p, BASIC_SPELL_STR, strlen(p)) != 0)
	    {
	      callchain_msg(
	        "oops, something went wrong when reading from gdb\n");
	      pclose(gdb_pipe);
	      error = 1;
	      goto SAVE_EXIT_00;
	    }
          else
            {
#	      ifdef DEBUG_GDB_PIPE
	        callchain_msg("undump: found sentinel\n");
#	      endif
            }
	}

      pclose(gdb_pipe);
    }
  else 
    {
      fputs(" @$#!|&^              |\n", logfile);
      callchain_msg("could not open pipe to gdb\n");
      error = 1;
      goto SAVE_EXIT_00;
    }
    
SAVE_EXIT_00:

  remove(tmp_file_name);
  free(cmd);
  return error;
}

/* ------------------------------------------------------------------------ */
#endif /* USE_BIDIRECTIONAL_CODE */
/* ------------------------------------------------------------------------ */

static void insert_file_info_in_chains()
{
  int error = 0;

  if(!file_info) return;

  fputs("| reading file info from gdb ...", logfile);

# ifdef USE_BIDIRECTIONAL_CODE
    (void) error;
    insert_file_info_in_chains_from_bidirectional_pipe();
# else
    error = insert_file_info_in_chains_from_unidirectional_pipe(0);
# endif

  if(!error) fputs(" done.                |\n", logfile);
  fflush(logfile);
}

/* ------------------------------------------------------------------------ */

/* this does the `pretty printing' of callchains'
 */

/* ------------------------------------------------------------------------ */

static void tab(FILE* file, int t, char c)
{
  int i;
  for(i=0; i<t; i++) putc(c, file);
}

static void _print_chain(FILE* file, CallChain c,
  char * prefix, char * lastprefix, char * lastlastprefix,
  int first_indent, int other_indent)
{
  if(empty_lines)
    {
      fputs(prefix, file);
      putc('\n', file);
    }

  if(c==0)
    {
      if(lastprefix) fputs(lastprefix, file);
      tab(file, first_indent, '-');
      fputs("> 0x????????\n", file);
    }
  else
    {
      int not_first = 0;

      if(c -> key.next) 
        {
	  if(prefix) fputs(prefix, file);
          tab(file, first_indent + 2, ' ');
        }
      else
        {
	  if(lastprefix) fputs(lastprefix, file);
          tab(file, first_indent, '-');
	  fputs("> ", file);
        }

      do
        {
	  char * name;
	  
	  if(!c -> addr_data)
	    name = "???";
	  else
	  if(c -> addr_data -> alternative_name)
	    name = c -> addr_data -> alternative_name;
	  else
	  if(c -> addr_data -> name)
	    name = c -> addr_data -> name;
	  else name = "???";
	   
#         ifdef OS_IS_SUNOS
            if(*name=='_') name++;
#         endif

	  if(c -> key.addr) fprintf(file, "0x%08lx ", (long) c -> key.addr);
	  else fputs("0x???????? ", file);

	  if(!print_on_one_line) fputs("in ", file);

	  putc('<', file);
	  fputs(name, file);
	  putc('>', file);

	  if(c -> addr_data && c -> addr_data -> file)
	    {
              if(!print_on_one_line)
                {
	          putc('\n', file);

	          if(c -> key.next)
	            {
	              if(prefix) fputs(prefix, file);
		    }
	          else
	            {
		      if(lastlastprefix) fputs(lastlastprefix, file);
		    }

                  tab(file,
		    (not_first ? other_indent : first_indent) + 2, ' ');

		       /*0x12345678 in*/
	          fputs("           ", file);

	          fprintf(file, "at %s:%d",
		    c -> addr_data -> file, c -> addr_data -> lineno);
	        }
  	      else
                {
	          fprintf(file,
		" %s:%d", c -> addr_data -> file, c -> addr_data -> lineno);
		}
	    }

	  c = c -> key.next;

	  if(c)
	    {
	      putc('\n', file);
              if(empty_lines)
	        {
		  fputs(prefix, file);
		  putc('\n', file);
		}

	      if(!not_first) not_first = 1;

	      if(c -> key.next != 0)
	        {
	          if(prefix) fputs(prefix, file);
                  tab(file,
		    (not_first ? other_indent : first_indent) + 2, ' ');
		}
	      else
	        {
	          if(lastprefix) fputs(lastprefix, file);
                  tab(file, (not_first ? other_indent : first_indent), '-');
		  fputs("> ", file);
		}
	    }
	}
      while(c);

      putc('\n', file);
      if(empty_lines)
	{
	  fputs(lastlastprefix, file);
	  putc('\n', file);
	}
    }
}

/* ------------------------------------------------------------------------ */

/* store a new Symbol_ in the symbol table `symtab'. Do not overwrite
 * already defined symbols.
 */

static void
new_Symbol(char * name, caddr_t addr)
{
  Symbol * symbol = (Symbol*) malloc(sizeof(Symbol));
  symbol -> name = name;
  symbol -> addr = addr;

  if(insert_into_HashTable(symtab, symbol))
    {
      if(debug_flag)
        {
          callchain_msg("multiple occurence of symbol %s at address 0x%08x\n",
      	                symbol -> name, (unsigned)symbol -> addr);
          free(symbol);
        }
    }
}

/* ------------------------------------------------------------------------ */

/* this is `nm' specifique
 */

static int
is_valid_symbol_type(char * p)
{
  switch(p[0])
    {
      case 't': case 'T': case 'w': case 'W': return ! p[1];
      default: return 0;
    }
}

/* ------------------------------------------------------------------------ */

static int
is_valid_symbol_name(const char *s)
{
  return strchr(s, '.') == NULL;
}

/* ------------------------------------------------------------------------ */

/* `parseNmLine' reads a line from `nm' with the expected syntax mentioned
 * in the function body. If the line conforms to the expected syntax
 * and the symbol described in that line has the right type (as defined
 * by `is_valid_symbol_type' than it is inserted in the symbol table 
 * (symtab) with `new_Symbol'. Otherwise nothing happens. Especially if
 * the line does not conform to the expected syntax than this line is
 * skipped *silently*.
 *
 * On the fly we also check if the symbols belong to currently running
 * program. If we found a counterexample we return false. Otherwise
 * we always return true.
 */

/* Only the symbols `main', `ccmalloc_malloc', and `ccmalloc_free' are
 * tested:
 */

extern int main();
extern void * ccmalloc_malloc(size_t);
extern void ccmalloc_free(void *);

#ifdef OS_IS_SUNOS
#  define MAIN			"_main"
#  define CCMALLOC_MALLOC	"_ccmalloc_malloc"
#  define CCMALLOC_FREE		"_ccmalloc_free"
#else
#  define MAIN			"main"
#  define CCMALLOC_MALLOC	"ccmalloc_malloc"
#  define CCMALLOC_FREE		"ccmalloc_free"
#endif

static int symtab_contains_important_symbols()
{
  HashTableIterator it;
  int found_main = 0, found_malloc = 0, found_free = 0;

  for(setup_HashTableIterator(symtab,&it);
      !is_done_HashTableIterator(&it);
      increment_HashTableIterator(&it))
    {
      Symbol * s = (Symbol*) get_data_from_HashTableIterator(&it);

      if(strcmp(s -> name, MAIN)==0) found_main = 1;
      else
      if(strcmp(s -> name, CCMALLOC_MALLOC)==0) found_malloc = 1;
      else
      if(strcmp(s -> name, CCMALLOC_FREE)==0) found_free = 1;
    }
  
  return found_main && found_malloc && found_free;
}

static int parseNmLine(char * buffer)
{
  /* syntax of a line: ["0x"]<hexnumber><space><string><space><C++-proto> */

  char * p, * name;
  caddr_t addr;

  if(!(p = strtok(buffer, " \t"))) return 1;
  if(p[0] == '0' && p[1] == 'x') p += 2;	/* skip '0x' (defensive) */
  addr = (caddr_t) strtol(p, 0, 16);

  if(!(p = strtok(0, " \t"))) return 1;
  if(!is_valid_symbol_type(p)) return 1;
  if(!(p = strtok(0, ""))) return 1;
  if(!is_valid_symbol_name(p)) return 1;

  if((strcmp(p, MAIN) == 0 && addr != (caddr_t) main) ||
     (strcmp(p, CCMALLOC_FREE) == 0 && addr != (caddr_t) ccmalloc_free) ||
     (strcmp(p, CCMALLOC_MALLOC) == 0 && addr != (caddr_t) ccmalloc_malloc))
    return 0;

  name = find_string(p);
  new_Symbol(name, addr);

  return 1;
}

/* ------------------------------------------------------------------------ */

const char * nm_options =

#ifdef OS_IS_SOLARIS	/* SunOS 5.X */

  /* -C   demangle C++ identifiers
   * -p   sparse output like on sunos 4.3.1
   * -x   hexadezimal output with leading '0x'
   *      used because nm for sunos has hexadezimal
   *      output (but without leading '0x')
   */

  "-C -p -x";

#else		
#ifdef OS_IS_SUNOS	/* SunOS 4.X */

  /* No demangling 8-( */

  "";

#else
#ifdef OS_IS_LINUX

  /* -C               demangle C++ identifiers
   * --defined-only   we do not need external symbols
   *
   *  default for linux is hexadezimal output
   */

  "-C --defined-only";

#else
#ifdef OS_IS_OSF1	/* only compiled on alpha (port does not work yet)
                         */

  /* -B Berkley style
   * -t x hexadezimal
   */

  "-B -t x";

#else

  /* This is the least denominator. */

  "";

#endif	/* OS_IS_OSF1 */
#endif	/* OS_IS_LINUX */
#endif	/* OS_IS_SUNOS */
#endif	/* OS_IS_SOLARIS */

/* ------------------------------------------------------------------------ */

/* Opens a pipe to `nm' with the file name `name' as argument, reads from
 * this pipe all text symbols and stores them in the symbol table `symtab'.
 * It returns true if the file with the given name exists and the symbols
 * in this file match. 
 */

static int get_symtab(char * name)
{
  FILE * pipe;
  int right_file = 0;

  sprintf(buffer, "nm %s %s 2>/dev/null", nm_options, name);

  if((pipe = popen(buffer, "r")) == 0)
    callchain_fatal("get_symtab: could not execute `%s'\n", buffer);

  /* prepare symtab */

  while(readLine(pipe, buffer, MAXBUFFER) &&
        (right_file = parseNmLine(buffer)))
    ;
  
  pclose(pipe);

  return right_file && symtab_contains_important_symbols();
}

/* ------------------------------------------------------------------------ */

/* support functions for symbol table `symtab' and sorted array symbols
 */

static int cmp_symbol_addr_qsort(const void * _a, const void * _b)
{
  Symbol * a = *(Symbol**) _a, * b = *(Symbol**) _b;

  if(a== b) return 0;

  if(a -> addr != b -> addr)
    {
      if(a -> addr < b -> addr)
        {
          return -1;
        }
      else return 1;
    }
  else return strcmp(a -> name, b -> name);
}

/* ------------------------------------------------------------------------ */

static int cmp_symbol_names(const void * _a, const void * _b)
{
  Symbol * a = (Symbol*) _a, * b = (Symbol*) _b;
  int cmp;

  if(a == b) return 0;

  if(a -> name == b -> name || (cmp = strcmp(a -> name, b -> name)) == 0)
    {
      if(a -> addr != b -> addr)
        {
          if(a -> addr < b -> addr)
            {
      	return -1;
            }
          else return 1;
        }
      else return 0;
    }
  else return cmp;
}

/* ------------------------------------------------------------------------ */

static void _freeSymbol(Symbol * s) { free(s); }

/* ------------------------------------------------------------------------ */

static int _hash_Symbol(Symbol * s)
{
  return hashpjw_HashTable(s -> name) ^ ((int)s -> addr);
}

/* ------------------------------------------------------------------------ */

/* wrap them in appropriate cast macros */

#define cmpNames ((int(*)(void*,void*)) cmp_symbol_names)
#define hashName ((int(*)(void*))_hash_Symbol)
#define freeSymbol ((void(*)(void*)) _freeSymbol)

/* ------------------------------------------------------------------------ */

/* Initializes the sorted array `symbols' from the symbol table `symtab'.
 * `symbols' is sorted with respect to the address of the symbols.
 * This enables fast access to a symbol through its address
 * (compare with `name_chain').
 */

static void copy_symtab_into_symbols()
{
  HashTableIterator it;
  int pos;

  if(!symtab || !(num_symbols = get_size_of_HashTable(symtab))) return;

  symbols = (Symbol**) malloc(sizeof(Symbol*) * (num_symbols + 1));

  for(pos=0, setup_HashTableIterator(symtab,&it);
      !is_done_HashTableIterator(&it);
      increment_HashTableIterator(&it), pos++)
    {
      Symbol * s = (Symbol*) get_data_from_HashTableIterator(&it);
      symbols[pos] = s;
    }
  
  assert(pos == num_symbols);
  symbols[pos] = 0;

  qsort(symbols, num_symbols, sizeof(Symbol*), cmp_symbol_addr_qsort);
}

/* ------------------------------------------------------------------------ */

/* Install `entry_start', `entry_end', `text_start', and `text_end'. These
 * addresses are used by `backtrace()' to find the entry point of a
 * callchain with the macro `isEntryPc'.
 */

static void get_bounds()
{
  int i;

  if(symtab && num_symbols>0)
    {
      for(i=0; i<num_symbols; i++)
        {
#ifdef OS_IS_SUNOS
          if(strcmp(symbols[i] -> name, "start") == 0)
#else
          if(strcmp(symbols[i] -> name, "_start") == 0)
#endif
            {
              entry_start = symbols[i++] -> addr;
              entry_end = symbols[i] -> addr;
              break;
	    }
        }

      text_start = symbols[0] -> addr;
      text_end = symbols[num_symbols-1] -> addr;

      have_bounds = 1;
    }
  else have_bounds = 0;
}

/* ------------------------------------------------------------------------ */

static void test_for_garbage_at_eol()
{
  char * buffer;
  if((buffer = strtok(0, " \t")))
    callchain_error("%s:%d: garbage at end of line\n",
       startup_file_name, startup_file_lineno);
}

/* ------------------------------------------------------------------------ */

/* Parses a number argument of a command in the startup file with the
 * given name (for error reporting) and stores the value of the parsed string
 * in the flag. If the buffer is empty then 1 is used.
 */

static void parseNumberFlag(char * buffer, char * name, int * flag)
{
  (void) name;

  if((buffer = strtok(0, " \t")))
    {
      *flag = atoi(buffer);
      test_for_garbage_at_eol();
    }
  else *flag = 1;
}

/* ------------------------------------------------------------------------ */

/* Parses a boolean argument of a command in the startup file with the
 * given name (for error reporting) and stores the value of the parsed string
 * in the flag. If the buffer is empty then the flag is set to 1 (true).
 */

static void parseBooleanFlag(char * buffer, char * name, int * flag)
{
  if(!(buffer = strtok(0, " \t"))) *flag = 1;
  else
    {
      if(strcmp(buffer, "1") == 0)
        {
	  *flag = 1;
	  test_for_garbage_at_eol();
	}
      else
      if(strcmp(buffer, "0") == 0)
        {
	  *flag = 0;
	  test_for_garbage_at_eol();
	}
      else
        callchain_error(
	  "%s:%d: expected boolean argument for `%s'\n",
	  startup_file_name, startup_file_lineno, name);
    }
}

/* ------------------------------------------------------------------------ */

/* Parses a call chain specification a la dont-log-chain and only-log-chain.
 */
static void parseCallChain(char *buffer, int *num_chains, 
			   DontLogChain ***chains)
{
  char *skip_name, *end_of_name, *colon;
  DontLogChain *chain;
  int num;
  (*num_chains)++;
  if(*num_chains > 1)
    {
      DontLogChain **nw = (DontLogChain **)malloc(
					    *num_chains*sizeof(DontLogChain*));
      memcpy(nw, *chains, (*num_chains-1)*sizeof(DontLogChain*));
      free(*chains);
      *chains = nw;
    }
  else
    {
      *chains = (DontLogChain **)malloc(sizeof(DontLogChain*));
    }
  num = 1;
  skip_name = buffer;
  while((skip_name = strchr(skip_name, ',')))
    {
      skip_name++;
      num++;
    }
  (*chains)[*num_chains - 1] = 
    (DontLogChain *)malloc(sizeof(DontLogChain) + 
			   (num-1)*sizeof(FuncOrFile));
  chain = (*chains)[*num_chains -1];
  chain->num = num;
  skip_name = buffer;
  end_of_name = buffer;
  num = 0;
  do
    {
      end_of_name = strchr(end_of_name, ',');
      if(end_of_name)
	*end_of_name='\0';
      colon = strchr(skip_name, ':');
      if(colon)
	{
	  *colon = '\0';
	  colon++;
	  chain->funcs[num].line = atoi(colon);
	}
      else
	chain->funcs[num].line = -1;
      chain->funcs[num].name = find_string(skip_name);
      if(end_of_name)
	end_of_name++;
      skip_name = end_of_name;
      num++;
    }
  while(end_of_name);
}

/* ------------------------------------------------------------------------ */

/* This is the parser of a line read from the startup file.
 */

static void parseStartupFileLine(char * buffer)
{
  startup_file_lineno++;

  if(!(buffer = strtok(buffer, " \t"))) return;	/* just skip empty lines */

  if(buffer[0] == '%' || buffer[0] == '#') return;	/* skip comments */

  if(strcmp(buffer, "file") == 0)
    {
      if(!(buffer = strtok(0, " \t")))
        callchain_msg(
	  "%s:%d: argument to `file' is missing\n",
	   startup_file_name, startup_file_lineno);
      else
        {
          file_name = find_string(buffer);
	  test_for_garbage_at_eol();
        }
    }
  else
  if(strcmp(buffer, "log") == 0)
    {
      if(logfilename)
        callchain_msg(
	  "%s:%d: Use only one of log or logpid\n",
	  startup_file_name, startup_file_lineno);

      if(!(buffer = strtok(0, " \t")))
        callchain_msg(
	  "%s:%d: argument to `log' is missing\n",
	  startup_file_name, startup_file_lineno);
      else
        {
	  int l = strlen(buffer);

	  if(l >= 3 && strcmp(buffer + l - 3, ".gz") == 0)
	    compressed_logfile = GZIP_LOGFILE;
	  else
	  if(l >= 2 && strcmp(buffer + l - 2, ".Z") == 0)
	    compressed_logfile = COMPRESS_LOGFILE;
	  else compressed_logfile = NO_COMPRESSED_LOGFILE;

	  logfilename = find_string(buffer);
	  test_for_garbage_at_eol();
	}
    }
  else
  if(strcmp(buffer, "logpid") == 0)
    {
      if(logfilename)
        callchain_msg(
	  "%s:%d: Use only one of log or logpid\n",
	  startup_file_name, startup_file_lineno);

      if(!(buffer = strtok(0, " \t")))
        callchain_msg(
	  "%s:%d: argument to `logpid' is missing\n",
	  startup_file_name, startup_file_lineno);
      else
        {
	  int l = strlen(buffer);
	  char *pid_buf;

	  if(l >= 3 && strcmp(buffer + l - 3, ".gz") == 0)
	    compressed_logfile = GZIP_LOGFILE;
	  else
	  if(l >= 2 && strcmp(buffer + l - 2, ".Z") == 0)
	    compressed_logfile = COMPRESS_LOGFILE;
	  else compressed_logfile = NO_COMPRESSED_LOGFILE;

	  pid_buf = find_string(buffer);
	  l = strlen(pid_buf);
	  /* pid < 9 digits ! */
	  logfilename = (char *) malloc((l + 10) * sizeof(char));
	  strcpy(logfilename, pid_buf);
	  sprintf(pid_buf, ".%d", (int) getpid());

	  switch(compressed_logfile)
	  {
	  case GZIP_LOGFILE:
	    strcpy(logfilename + l - 3, pid_buf);
	    strcat(logfilename, ".gz");
	    break;
	  case COMPRESS_LOGFILE:
	    strcpy(logfilename + l - 2, pid_buf);
	    strcat(logfilename, ".Z");
	    break;
	  default:
	    strcat(logfilename, pid_buf);
	    break;
	  }

	  test_for_garbage_at_eol();
	}
    }
  else
  if(strcmp(buffer, "only-log-chain") == 0)
    {
      if(!(buffer = strtok(0, " \t")))
	{
	  /* No arguments! */
	  parseCallChain("", &num_only_log_chains, &only_log_chains);
	}
      else
        {
	  parseCallChain(buffer, &num_only_log_chains, &only_log_chains);
	  test_for_garbage_at_eol();
	}
    }
  else
  if(strcmp(buffer, "dont-log-chain") == 0)
    {
      if(!(buffer = strtok(0, " \t")))
	{
	  /* No arguments! */
	  parseCallChain("", &num_dont_log_chains, &dont_log_chains);
	}
      else
        {	  
	  parseCallChain(buffer, &num_dont_log_chains, &dont_log_chains);
	  test_for_garbage_at_eol();
	}
    }
  else
  if(strcmp(buffer, "set") == 0)
    {
      if(!(buffer = strtok(0, " \t")))
        callchain_error(
	  "%s:%d: argument to `set' is missing\n",
	  startup_file_name, startup_file_lineno);
      else
        {
          if(strcmp(buffer, "keep-deallocated-data") == 0)
            parseBooleanFlag(buffer,
	      "keep-deallocated-data", &keep_deallocated_data);
          else
	  if(strcmp(buffer, "load-dynlibs") == 0)
	    parseBooleanFlag(
	      buffer, "load-dynlibs", &load_dynlibs_with_gdb);
	  else
	  if(strcmp(buffer, "align-8-byte") == 0)
	    parseBooleanFlag(
	      buffer, "align-8-byte", &align_on_8_byte_boundary);
	  else
          if(strcmp(buffer, "statistics") == 0)
            parseBooleanFlag(buffer, "statistics", &callchain_statistics);
          else
          if(strcmp(buffer, "only-count") == 0)
            parseBooleanFlag(buffer, "only-count", &only_count);
          else
          if(strcmp(buffer, "silent") == 0)
            parseBooleanFlag(buffer, "silent", &silent_startup);
          else
          if(strcmp(buffer, "print-on-one-line") == 0)
            parseBooleanFlag(buffer, "print-on-one-line", &print_on_one_line);
          else
          if(strcmp(buffer, "additional-line") == 0)
            parseBooleanFlag(buffer, "additional-line", &empty_lines);
          else
          if(strcmp(buffer, "chain-length") == 0)
            parseNumberFlag(buffer, "chain-length", &chain_length);
          else
          if(strcmp(buffer, "sort-by-size") == 0)
            parseBooleanFlag(buffer, "sort-by-size", &sort_by_size);
          else
          if(strcmp(buffer, "sort-by-wasted") == 0)
            parseBooleanFlag(buffer, "sort-by-wasted", &sort_by_wasted);
          else
          if(strcmp(buffer, "print-addresses") == 0)
            parseBooleanFlag(buffer, "print-addresses", &print_addresses_flag);
          else
          if(strcmp(buffer, "check-overwrites") == 0)
            parseNumberFlag(buffer, "check-overwrites", &check_overwrites);
          else
          if(strcmp(buffer, "check-underwrites") == 0)
            parseNumberFlag(buffer, "check-underwrites", &check_underwrites);
          else
          if(strcmp(buffer, "check-free-space") == 0)
            parseBooleanFlag(buffer, "check-free-space", &check_free_space);
          else
          if(strcmp(buffer, "check-start") == 0)
            parseNumberFlag(buffer, "check-start", &check_start);
          else
          if(strcmp(buffer, "check-interval") == 0)
            parseNumberFlag(buffer, "check-interval", &check_interval);
          else
          if(strcmp(buffer, "file-info") == 0)
            parseBooleanFlag(buffer, "file-info", &file_info);
	  else
	  if(strcmp(buffer, "debug") == 0)
	    parseBooleanFlag(buffer, "debug", &debug_flag);
	  else
	  if(strcmp(buffer, "library-chains") == 0)
	    parseBooleanFlag(buffer, "library-chains", &library_chains);
	  else
	  if(strcmp(buffer, "continue") == 0)
	    parseBooleanFlag(buffer, "continue", &continue_flag);
          else callchain_msg(
	    "%s:%d: unknown flag `%s' for `set'\n",
	     startup_file_name, startup_file_lineno, buffer);
        }
    }
  else
    callchain_msg(
      "%s:%d: unknown command `%s'\n",
      startup_file_name, startup_file_lineno, buffer);
}

/* ------------------------------------------------------------------------ */

static void open_logfile()
{
  if(compressed_logfile == NO_COMPRESSED_LOGFILE)
    {
      if(logfilename)
        {
          if(strcmp(logfilename, "-") == 0 ||
	     strcmp(logfilename, "stdout") == 0)
            {
	      logfile = stdout;
	    }
          else
            {
              logfile = fopen(logfilename, "w");
              if(!logfilename)
                {
	          callchain_msg("could not open `%s' as log file\n",
		                logfilename);
	          logfile = stderr;
	        }
	    }
        }
    }
  else
    {
      char * cmd_buffer;
      const char * compress_cmd;

      if(compressed_logfile == GZIP_LOGFILE) compress_cmd = "gzip -c";
      else compress_cmd = "compress";

      assert(logfilename);

      cmd_buffer = (char*) malloc(strlen(logfilename) + 30);
      sprintf(cmd_buffer, "%s > %s", compress_cmd, logfilename);

      logfile = popen(cmd_buffer, "w");
      if(!logfile)
        {
          callchain_msg("could not open `%s' (using stderr)\n", cmd_buffer);
          logfile = stderr;
          compressed_logfile = NO_COMPRESSED_LOGFILE;
        }

      free(cmd_buffer);
    }
}

/* ------------------------------------------------------------------------ */

static void close_logfile()
{
  if(compressed_logfile == NO_COMPRESSED_LOGFILE)
    {
      if(logfile != stderr && logfile != stdout) fclose(logfile);
    }
  else pclose(logfile);
}

/* ------------------------------------------------------------------------ */
/* `read_startup_file' is the first function executed by this library.
 */ 

static void read_startup_file()
{
  FILE * startup_file = fopen(startup_file_name, "r");

  if(!startup_file)
    {
      char * home = getenv("HOME");

      if(!home) return;

      if(strlen(home) + strlen(startup_file_name) + 2 >= MAXBUFFER) return;

      strcpy(buffer, home);
      strcat(buffer, "/");
      strcat(buffer, startup_file_name);

      startup_file = fopen(buffer, "r");

      if(!startup_file)
        {
          startup_file_name = ".ccmalloc (but not found)";
        }
      else startup_file_name = "~/.ccmalloc";
    }

  if(startup_file)
    {
      while(readLine(startup_file, buffer, MAXBUFFER))
        parseStartupFileLine(buffer);
      
      fclose(startup_file);
    }
  
  open_logfile();
}

/* ------------------------------------------------------------------------ */

static void banner()
{
  char * lfn, * date_str, * osinfo_str;
  time_t t;
  struct utsname osinfo;

  lfn = logfilename ? logfilename : "stderr";

  (void) time(&t);
  date_str = strtok(ctime(&t), "\n");

  uname(&osinfo);
  osinfo_str=(char*) malloc(
    strlen(osinfo.sysname) + 1 +
    strlen(osinfo.release) + 1 +
    strlen(osinfo.machine) + 1 +
    strlen("on") + 1 +
    strlen(osinfo.nodename) + 1);
  
  sprintf(osinfo_str, "%s %s %s on %s",
    osinfo.sysname, osinfo.release, osinfo.machine, osinfo.nodename);

  fputs(
".--------------------------------------------------------------------------.\n"
"|================ ccmalloc-0.2.3 (C) 1997-1998 Armin Biere ================|\n"
"+--------------------------------------------------------------------------+\n",
    logfile);
  fprintf(logfile,
"| executable       = %-53s |\n", file_name ? file_name : "a.out");
  fprintf(logfile,
"| startup file     = %-53s |\n", startup_file_name);
  fprintf(logfile,
"| log file         = %-53s |\n", lfn);
  fprintf(logfile,
"| start time       = %-53s |\n", date_str);
  fprintf(logfile,
"| operating system = %-53s |\n", osinfo_str);

  free(osinfo_str);

  fprintf(logfile,
"+--------------------------------------------------------------------------+\n");
  fprintf(logfile,
"| only-count        = %-11d  keep-deallocated-data = %-15d |\n",
   only_count,             keep_deallocated_data);
  fprintf(logfile,
"| check-interval    = %-11d  check-free-space      = %-15d |\n",
   check_interval,            check_free_space);
  fprintf(logfile,
"| check-start       = %-11d  file-info             = %-15d |\n",
   check_start,               file_info);
  fprintf(logfile,
"| chain-length      = %-11d  additional-line       = %-15d |\n",
   chain_length,              empty_lines);
  fprintf(logfile,
"| check-underwrites = %-11d  print-addresses       = %-15d |\n", 
   check_underwrites,         print_addresses_flag);
  fprintf(logfile,
"| check-overwrites  = %-11d  print-on-one-line     = %-15d |\n",
   check_overwrites,          print_on_one_line);
  fprintf(logfile,
"| sort-by-wasted    = %-11d  sort-by-size          = %-15d |\n",
   sort_by_wasted,            sort_by_size);
  fprintf(logfile,
"| # only-log-chain  = %-11d  continue              = %-15d |\n",
   num_only_log_chains,       continue_flag);
  fprintf(logfile,
"| # dont-log-chain  = %-11d  statistics            = %-15d |\n",
   num_dont_log_chains,       callchain_statistics);
  fprintf(logfile,
"| debug             = %-11d  library-chains        = %-15d |\n",
   debug_flag,                library_chains);
  fprintf(logfile,
"| load-dynlibs      = %-11d  align-8-byte          = %-15d |\n",
   load_dynlibs_with_gdb,     align_on_8_byte_boundary);
  fputs(
"`--------------------------------------------------------------------------'\n",
          logfile);
}

/* ------------------------------------------------------------------------ */

static void search_cwd_for_executable()
{
  /* search current directory for matching executable */

  DIR * dir;
  callchain_msg("trying to find executable in current directory ...\n");
  dir = opendir(".");

  for(;;)
    {
      struct dirent * e;

      if((e = readdir(dir)) == NULL)
        {
          closedir(dir);
          free_HashTable(symtab);
          symtab = 0;
          break;
        }
      else
        {
          struct stat s;
          stat(e -> d_name, &s);
          if(!(S_ISDIR(s.st_mode)))
            {
              if((s.st_mode & S_IFREG) &&
                 (s.st_mode & S_IREAD) &&
	         (s.st_mode & S_IEXEC))
	        {
                  free_HashTable(symtab);
                  symtab = new_HashTable(
	            100, cmpNames, hashName, 0, freeSymbol, 0);
	      
	          if(get_symtab(e -> d_name))
	            {
		      copy_symtab_into_symbols();
		      file_name = find_string(e -> d_name);
		      closedir(dir);
		      if(silent_startup)
		        callchain_msg("using symbols from `%s'\n", file_name);
		      break;
		    }
	        }
            }
        }
      
    }
  
  /* as the call to `search_cwd_for_executable' should always be the last
   * option we can report a failure here
   */

  if(symtab)
    {
      assert(file_name);

      callchain_msg("(to speed up this search specify `file %s'\n", file_name);
      callchain_msg(" in the startup file `%s')\n", startup_file_name);
    }
  else
    {
      callchain_msg("Could not find an executable with valid symbols\n");
      callchain_msg("Specify it with the `file' command\n");
      callchain_msg("in the startup file `%s'\n", startup_file_name);
      callchain_msg("or link `a.out' to this executable!\n");
    }
}

/* ------------------------------------------------------------------------ */
#ifdef CAN_GET_ARGV0
/* ------------------------------------------------------------------------ */
#ifdef OS_IS_LINUX
/* ------------------------------------------------------------------------ */
/* !!!!!uses the global buffer!!!!!
 */

static char * find_argv0()
{
  char my_cmdline_args_name[200];
  FILE *  my_cmdline_args_file;

  sprintf(my_cmdline_args_name, "/proc/%d/cmdline", getpid());
  my_cmdline_args_file = fopen(my_cmdline_args_name, "r");
  if(!my_cmdline_args_file)
    {
      callchain_msg("could not open `%s'\n", my_cmdline_args_name);
      return 0;
    }
  
  if(!readLine(my_cmdline_args_file, buffer, MAXBUFFER) ||
     strlen(buffer) == 0)
    {
      callchain_msg("could not read `argv[0]' from `%s'\n",
                      my_cmdline_args_name);
      fclose(my_cmdline_args_file);
      return 0;
    }
  
  fclose(my_cmdline_args_file);
  return find_string(buffer);
}

/* ------------------------------------------------------------------------ */
#else
#ifdef OS_IS_SOLARIS
/* ------------------------------------------------------------------------ */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/procfs.h>

static struct prpsinfo procfs_info;

static char * find_argv0()
{
  char procfs_name[200];
  int fd;
  pid_t pid = getpid();
  sprintf(procfs_name, "/proc/%d", (int) pid);
  fd = open(procfs_name, O_RDONLY);
  if(fd==-1)
    {
      callchain_msg("could not open `%s'\n", procfs_name);
      return 0;
    }

  if(ioctl(fd, PIOCPSINFO, &procfs_info)==-1)  
    {
      callchain_msg("could not get argv[0]' from `%s'\n", procfs_name);
      close(fd);
      return 0;
    }

  close(fd);
  if(!procfs_info.pr_argv[0] ||
     strlen(procfs_info.pr_argv[0]) == 0) return 0;
  else return find_string(procfs_info.pr_argv[0]);
}

/* ------------------------------------------------------------------------ */
#endif	/* OS_IS_SOLARIS */
#endif	/* OS_IS_LINUX */
#endif	/* CAN_GET_ARGV0 */
/* ------------------------------------------------------------------------ */

static void init_CallChain()
{
  symtab = new_HashTable(100, cmpNames, hashName, 0, freeSymbol, 0);

# ifdef CAN_GET_ARGV0
    {
      int match;
      char * argv0 = find_argv0();
      
      /* now test if `argv0' and `file_name' match */

      if(argv0)
        {
          if(file_name)
            {
	      if(argv0[0] == '/' && file_name[0] == '/')
                {
	          /* `match=(argv0==file_name)' is not defensive */

	          match = (strcmp(argv0, file_name) == 0);
	        }
              else
                {
	          char * p = rindex(argv0, '/');
	          char * q = rindex(file_name, '/');

	          if(p) p++; else p = argv0;
	          if(q) q++; else q = file_name;
    
                  match = (strcmp(p,q) == 0);

	          /* preference to absolut path names */

	          if(match && argv0[0] == '/') file_name = argv0;
	        }
            }
          else
            {
	      /* make them match */

	      match = 1;
	      file_name = argv0;
	    }
	}
      else match = 1;  /* argv0 == 0 */

      if(match)
        {
          if(get_symtab(file_name)) copy_symtab_into_symbols();
	  else search_cwd_for_executable();
	}
      else
        {
          callchain_msg(
            "\n  file-name=%s and\n"
	    "  argv[0]=%s\n"
	    "  do not match (trying argv[0])\n",
            file_name, argv0);

          if(get_symtab(argv0))
            {
              file_name = argv0;
              copy_symtab_into_symbols();
            }
          else
            {
              callchain_msg(
                "argv[0]=%s does not contain valid symbols\n"
                "trying to use file-name=%s\n",
                argv0, file_name);

              if(get_symtab(file_name)) copy_symtab_into_symbols();
              else
                {
                  callchain_msg(
                    "file-name=%s does not contain valid symbols *too*\n",
       	            file_name);
                  search_cwd_for_executable();
                }
            }
	}
    }
# else
    {
      if(!file_name) file_name = find_string("a.out");

      if(get_symtab(file_name)) copy_symtab_into_symbols();
      else
        {
          callchain_msg(
	    "file-name=%s does not contain valid symbols\n", file_name);
	  search_cwd_for_executable();
        }
    }
# endif

  get_bounds();

  /* `empty_call_chain' is used if we can not generate a callchain with
   * `backtrace' ( <=> backtrace(..) == 0 ).
   */

  empty_call_chain = new_CallChain(0, 0);
}

/* ------------------------------------------------------------------------ */

/* In a program that trys to find memory leaks we should also be very
 * careful with reclaiming allocated memory -- at least from a moral point
 * of view ;-).
 */

static void exit_CallChain()
{
  int i;
  if(debug_flag)
    {
      if(symtab) callchain_msg(
        "[symtab: %s]\n", get_statistics_for_HashTable(symtab));
      if(chaintab) callchain_msg(
        "[chaintab: %s]\n", get_statistics_for_HashTable(chaintab));
      if(addrs) callchain_msg(
        "[addrs: %s]\n", get_statistics_for_HashTable(addrs));
      if(strings) callchain_msg(
        "[strings: %s]\n", get_statistics_for_HashTable(strings));
    }

  if(chaintab) free_HashTable(chaintab);
  if(chains) free(chains);
  if(addrs) free_HashTable(addrs);
  if(symtab) free_HashTable(symtab);
  if(symbols) free(symbols);
  if(strings) free_HashTable(strings);
  if(dont_log_chains) 
    {
      for (i = 0; i < num_dont_log_chains; i++)
	{
	  free(dont_log_chains[i]);
	}
      free(dont_log_chains);
    }
  if(only_log_chains) 
    {
      for (i = 0; i < num_only_log_chains; i++)
	{
	  free(only_log_chains[i]);
	}
      free(only_log_chains);
    }

  if(debug_flag) callchain_msg("[internal: %s]\n", wrapper_stats());
}

/* ------------------------------------------------------------------------ */

static void report_percent(float f)
{
  int a = (int) f, b = (int) ((f - (float)a)*10.0 + 0.5);
  fprintf(logfile, "%3d.%01d%%", a, b);
}

/* ------------------------------------------------------------------------ */

static void report_bytes(long n)
{
  float f = n;

  assert(n>=0);

  if(n<10240)
    {
      /*XXXXX Bytes*/

      fprintf(logfile, "%ld Bytes", n);
    }
  else
  if(n<1024*1024)
    {
      /*XXX.X KB   */

      fprintf(logfile, "%.1f KB", f / (float)1024);
    }
  else
  if(n<1024*1024*1024)
    {
      /*XXX.X MB   */

      fprintf(logfile, "%.1f MB", f / (float)(1024 * 1024));
    }
  else
    {
      /*XXX.X GB   */

      fprintf(logfile, "%.1f GB", f / (float)(1024 * 1024 * 1024));
    }
}

/* ------------------------------------------------------------------------ */

static long count_allocated(ccmallocLinks * q)
{
  long res;

  for(res=0; q; q=q->next)
    if(is_allocated_links(q)) res += get_size_links(q);
  
  return res;
}

/* ------------------------------------------------------------------------ */

static void print_addresses(
  FILE* file, ccmallocLinks * q, const char * prefix)
{
  int i = 0;

  while(q)
    {
      if(i==5)
        {
	  fprintf(file, "\n%s", prefix);
	  i=1;
	}
      else i++;

      fprintf(file, "%c0x%08x", is_allocated_links(q) ? '+' : '-',
              (unsigned)(((char*)q) + ccmallocLinks_ccmallocUserData_offset));
      if((q = q -> next)) putc(',', file);
    }
}

/* ------------------------------------------------------------------------ */

/* count number of allocations and deallocations */ 

static void
count_and_report_allocations(CallChain c)
{
  long _bytes_deallocated = 0, _bytes_allocated = 0,
       _num_allocated = 0, _num_deallocated = 0, _bytes_wasted = 0;

  ccmallocLinks * p;
  CCToplevelData * td = c -> toplevel_data;

  assert(td);

  _bytes_wasted = td -> bytes_wasted;

  for(p=td -> allocations; p; p = p -> next)
    {
      _num_allocated++;
      _bytes_allocated += get_size_links(p);

      if(!is_allocated_links(p))
	{
	  _num_deallocated++;
	  _bytes_deallocated += get_size_links(p);
	}
    }

#ifdef DEBUG
  if(callchain_statistics)
    {
      /* We also need the number of allocations ... so we
       * had to count again!
       */
       
      assert(_bytes_wasted == _bytes_allocated - _bytes_deallocated);
    }
#endif


  if(_bytes_allocated > 0 || callchain_statistics)
    {
      double percent = 0.0;
      long num_allocated_to_report = _num_allocated;
      
      if(keep_deallocated_data) num_allocated_to_report -= _num_deallocated;
     
      if(bytes_wasted != 0)
        percent = 100.0 * ((float)_bytes_wasted)/((float)bytes_wasted);

      if(percent >= 10.0) fputs("*", logfile);
      else fputs("|", logfile);

      report_percent(percent);
      fputs(" = ", logfile);
      report_bytes(_bytes_wasted);

      fprintf(logfile, " of garbage allocated in %ld allocation%s\n",
        num_allocated_to_report, num_allocated_to_report!=1 ? "s" : "");
    }
}
	 
/* ------------------------------------------------------------------------ */

static void report_callchain(CallChain c)
{
  CCToplevelData * td = c -> toplevel_data;

  assert(td);

  if(!ccmalloc_report_flag)
    {
      fputs(
"=======================================================\n", logfile);
      ccmalloc_report_flag = 1;

      if(empty_lines) fputs("|\n", logfile);
    }

  if(callchain_statistics)
    {
      /* In this case we also can report the total number of allocations
       * and deallocations for this `CallChain'.
       */
 
      CallChainStats * stats = td -> stats;

      count_and_report_allocations(c);

      if(stats -> num_allocated != 0)
        {
          fprintf(logfile,
"|       | %ld allocated (%ld Bytes = %3.1f%% of total allocated)\n",
            stats -> num_allocated, stats -> bytes_allocated,
            100.0 * ((float)stats -> bytes_allocated) /
	            ((float)bytes_allocated));
	}

      /* at least for the `empty_call_chain' we have both ... */

      if(stats -> num_deallocated != 0)
        {
          fprintf(logfile,
"|       | %ld deallocated (%ld Bytes = %3.1f%% of total allocated)\n",
            stats -> num_deallocated, stats -> bytes_deallocated,
            100.0 * ((float)stats -> bytes_deallocated) /
	            ((float)bytes_allocated));
	}
    }
  else count_and_report_allocations(c);

  if(print_addresses_flag && td -> allocations)
    {
      if(empty_lines) fputs("|       |\n", logfile);
      fputs("|       @ [ ", logfile);
      if(empty_lines) fputs("\n|       |   ", logfile);
      print_addresses(logfile, td -> allocations, "|       |   ");
      if(empty_lines) fputs("\n|       | ]\n", logfile);
      else fputs(" ]\n", logfile);
    }
  
  _print_chain(logfile, c, "|       |", "|       `", "|        ", 5, 5);
}

/* ------------------------------------------------------------------------ */

static int cmp_strings( const char *a, const char *b )
{
  if ( a && b )
    {
      return strcmp( a, b );
    }
  else if ( a )
    return -1;
  else if ( b )
    return 1;
  else
    return 0;
}

static int cmp_CallChains_by_BytesWasted_for_Qsort(
  const void * _a, const void * _b)
{
  CallChain a = *(CallChain*) _a, b = *(CallChain*) _b;
  CCToplevelData * tda = a -> toplevel_data, * tdb = b -> toplevel_data;

  long m, n;

  assert(tda);
  assert(tdb);

  m = tda -> bytes_wasted;
  n = tdb -> bytes_wasted;

  if(m == n) 
    {
      while ( a && b )
	{
	  if ( a->addr_data && b->addr_data )
	    {
	      if ( ( n = cmp_strings( a->addr_data->name, 
				      b->addr_data->name ) ) )
		  return n;
	      if ( ( n = cmp_strings( a->addr_data->file, 
				      b->addr_data->file ) ) )
		return n;
	      if ( ( n = cmp_strings( a->addr_data->alternative_name, 
				      b->addr_data->alternative_name ) ) )
		return n;
	      if ( a->addr_data->lineno < b->addr_data->lineno )
		return -1;
	      if ( a->addr_data->lineno > b->addr_data->lineno )
		return 1;
	    }
	  else if ( a->addr_data )
	    return -1;
	  else if ( a->addr_data )
	    return 1;
	  if ( a->key.addr < b->key.addr )
	    return -1;
	  if ( b->key.addr < a->key.addr )
	    return 1;

	  a = a->key.next;
	  b = b->key.next;
	}
      if ( a )
	return -1;
      if ( b )
	return 1;
      return 0;
    }
  else return m < n ? 1 : -1;
}

/* ------------------------------------------------------------------------ */

static int cmp_CallChains_by_Size_for_Qsort(
  const void * _a, const void * _b)
{
  CallChain a = *(CallChain*) _a, b = *(CallChain*) _b;
  CallChainStats * s, * t;
  long m, n;

  assert(callchain_statistics);
  assert(a -> toplevel_data);
  assert(b -> toplevel_data);

  s = a -> toplevel_data -> stats;
  t = b -> toplevel_data -> stats;

  m = s -> bytes_allocated;
  n = t -> bytes_allocated;

  if(m == n) 
    {
      while ( a && b )
	{
	  if ( a->addr_data && b->addr_data )
	    {
	      if ( ( n = cmp_strings( a->addr_data->name, 
				      b->addr_data->name ) ) )
		  return n;
	      if ( ( n = cmp_strings( a->addr_data->file, 
				      b->addr_data->file ) ) )
		return n;
	      if ( ( n = cmp_strings( a->addr_data->alternative_name, 
				      b->addr_data->alternative_name ) ) )
		return n;
	      if ( a->addr_data->lineno < b->addr_data->lineno )
		return -1;
	      if ( a->addr_data->lineno > b->addr_data->lineno )
		return 1;
	    }
	  else if ( a->addr_data )
	    return -1;
	  else if ( a->addr_data )
	    return 1;
	  if ( a->key.addr < b->key.addr )
	    return -1;
	  if ( b->key.addr < a->key.addr )
	    return 1;

	  a = a->key.next;
	  b = b->key.next;
	}
      if ( a )
	return -1;
      if ( b )
	return 1;
      return 0;
    }
  else return m < n ? 1 : -1;
}

/* ------------------------------------------------------------------------ */

static int cmp_CallChains_by_NumAllocated_for_Qsort(
  const void * _a, const void * _b)
{
  CallChain a = *(CallChain*) _a, b = *(CallChain*) _b;
  CallChainStats * s, * t;
  long m, n;

  assert(callchain_statistics);
  assert(a -> toplevel_data);
  assert(b -> toplevel_data);

  s = a -> toplevel_data -> stats;
  t = b -> toplevel_data -> stats;

  m = s -> num_allocated;
  n = t -> num_allocated;

  if(m == n) 
    {
      while ( a && b )
	{
	  if ( a->addr_data && b->addr_data )
	    {
	      if ( ( n = cmp_strings( a->addr_data->name, 
				      b->addr_data->name ) ) )
		  return n;
	      if ( ( n = cmp_strings( a->addr_data->file, 
				      b->addr_data->file ) ) )
		return n;
	      if ( ( n = cmp_strings( a->addr_data->alternative_name, 
				      b->addr_data->alternative_name ) ) )
		return n;
	      if ( a->addr_data->lineno < b->addr_data->lineno )
		return -1;
	      if ( a->addr_data->lineno > b->addr_data->lineno )
		return 1;
	    }
	  else if ( a->addr_data )
	    return -1;
	  else if ( a->addr_data )
	    return 1;
	  if ( a->key.addr < b->key.addr )
	    return -1;
	  if ( b->key.addr < a->key.addr )
	    return 1;

	  a = a->key.next;
	  b = b->key.next;
	}
      if ( a )
	return -1;
      if ( b )
	return 1;
      return 0;
    }
  else return m < n ? 1 : -1;
}

/* ------------------------------------------------------------------------ */
/* We only use the simple heuristic that at least 3 frames of c must contain
 * a valid address. Otherwise c is assumed to be an library call chain
 */

static int is_library(CallChain c)
{
  if(c -> key.addr && !is_library_pc(c -> key.addr)) return 0;
  else
    {
      int num_zero = 0, length = 0;

      for(; c; c = c -> key.next)
        {
	  if(!c -> key.addr || is_library_pc(c -> key.addr)) num_zero++;
	  length++;
	}
      
      return length - num_zero < 3;
    }
}

/* ------------------------------------------------------------------------ */

static int check_log_chains_match(CallChain c, int num_chains,
				  DontLogChain **chains)
{
  int i, j, match;
  CallChain d;

  /* Loop through all the chains to be checked for a match. */
  for(i=0; i<num_chains; i++)
    {
      match = 1;
      /* Walk down both chains while we're still matching. */
      for(j=0, d = c; j<chains[i]->num && d && match; 
	  j++, d = d->key.next)
	{
	  if(d->addr_data)
	    {
	      char *name = d->addr_data->alternative_name;
	      char *matchname = chains[i]->funcs[j].name;
	      char *file;
	      int lineno = d->addr_data->lineno;
	      int matchline = chains[i]->funcs[j].line;
	      if(!name)
		name = d->addr_data->name;
	      file = d->addr_data->file;
	      if(!name && !file && !matchname[0])
		match = 1;
	      else if((name && strcmp(name, matchname)==0) ||
		      (file && strcmp(file, matchname)==0))
		{
		  match = lineno==-1 || matchline==-1 ||
		    lineno == matchline;
		}
	      else
		match = 0;
	    }
	  else
	      match = chains[i]->funcs[j].name[0] == '\0';
	}
      if(match && j==chains[i]->num)
	  return 1;
    }
  return 0;
}

static int check_dont_log_chain(CallChain c)
{
  CallChain d;
  int match;

  if(only_log_chains)
    {
      d = c;
      match = 0;
      /* Walk through the whole chain, checking for a match. */
      while(d && !match)
	{
	  if(check_log_chains_match(d, num_only_log_chains, only_log_chains))
	    {
	      match = 1;
	    }
	  d = d->key.next;
	}
      /* If we didn't find a match anywhere, dont log it. */
      if(!match)
	  return 1;
    }
  if(dont_log_chains)
    {
      d = c;
      /* Try at every position in the call chain. */
      while(d)
	{
	  if(check_log_chains_match(d, num_dont_log_chains, dont_log_chains))
	    return 1;
	  d = d->key.next;
	}
    }
  /* Well, as default, log it! */
  return 0;
}

/* ------------------------------------------------------------------------ */

static void copy_chaintab_into_chains()
{
  HashTableIterator it;
  int i;

  num_chains = 0;

  for(setup_HashTableIterator(chaintab, &it);
      !is_done_HashTableIterator(&it);
      increment_HashTableIterator(&it))
    {
      CallChain c = (CallChain) get_data_from_HashTableIterator(&it);

      if(c)
        {
          if(has_allocated_data(c))
            {
	      if(is_library(c))
	        {
	          if(library_chains) 
		    num_chains++;
	        }
	      else num_chains++;
	    }
	}
    }

  chains = (CallChain*) malloc((num_chains + 1) * sizeof(CallChain));

  for(i=0, setup_HashTableIterator(chaintab, &it);
      !is_done_HashTableIterator(&it);
      increment_HashTableIterator(&it))
    {
      CallChain c = (CallChain) get_data_from_HashTableIterator(&it);
      if(c &&
         has_allocated_data(c) &&
         (library_chains || (!library_chains && !is_library(c))))
        {
	  CCToplevelData * td = c -> toplevel_data;
	  assert(td);
	  chains[i++] = c;
	  td -> bytes_wasted = count_allocated(td -> allocations);
	}
    }
  
  assert(i == num_chains);
  chains[i] = 0;
}

static void sort_chains()
{
  int (*cmp)(const void *, const void *);
  const char * prestr, * poststr;

  if(sort_by_wasted || !callchain_statistics)
    {
      prestr="| sorting by number of not reclaimed bytes ...";
      poststr=" done.  |\n";
      cmp = cmp_CallChains_by_BytesWasted_for_Qsort;
    }
  else
  if(sort_by_size)
    {
      prestr="| sorting by number of allocated bytes ..."; 
      poststr=" done.      |\n";
      cmp = cmp_CallChains_by_Size_for_Qsort;
    }
  else
    {
      prestr="| sorting by number of allocations ...";
      poststr=" done.          |\n";
      cmp = cmp_CallChains_by_NumAllocated_for_Qsort;
    }

  fputs(prestr, logfile);
  fflush(logfile);

  qsort(chains, num_chains, sizeof(CallChain), cmp);

  fputs(poststr, logfile);
  fflush(logfile);
}

/* ------------------------------------------------------------------------ */

static void report_callchain_counts()
{
  HashTableIterator it;
  int count_library_chains = 0;
  int count_unlogged_chains = 0;
  int rep_num_chains = 0;

  for(setup_HashTableIterator(chaintab, &it);
      !is_done_HashTableIterator(&it);
      increment_HashTableIterator(&it))
    {
      CallChain c = (CallChain) get_data_from_HashTableIterator(&it);

      if(c)
        {
          if(has_allocated_data(c))
            {
              if(check_dont_log_chain(c))
		{
		  count_unlogged_chains++;
		}
	      else
	      if(is_library(c))
	        {
	          count_library_chains++;
	          if(library_chains) rep_num_chains++;
	        }
	      else rep_num_chains++;
	    }
	}
    }

  fprintf(logfile, "| number of call chains: %-28d |\n", num_chains);
  fprintf(logfile, "| number of ignored call chains: %-20d |\n", 
	  count_unlogged_chains);
  fprintf(logfile, "| number of reported call chains: %-19d |\n", 
	  rep_num_chains);
  fprintf(logfile, "| number of internal call chains: %-19d |\n",
          rep_num_chains - (library_chains ? count_library_chains : 0));
  fprintf(logfile, "| number of library call chains: %-20d |\n",
          count_library_chains);
}

/* ------------------------------------------------------------------------ */

static void report_callchains()
{
  int i;

  bytes_wasted = bytes_allocated - bytes_deallocated;

  for(i=0; i<num_chains; i++)
    if(!check_dont_log_chain(chains[i]))
      report_callchain(chains[i]);
}

/* ------------------------------------------------------------------------ */

static void print_callchain(CallChain c)
{
  if(state == INITIALIZED)
    {
      name_chain(c);

#     ifdef USE_BIDIRECTIONAL_CODE
        {
          call_gdb();
          insert_file_info_in_chain_from_bidirectional_pipe(c);
          shut_down_gdb();
	}
#     else
        {
	  assert(c);
	  insert_file_info_in_chains_from_unidirectional_pipe(c);
	}
#     endif
    }

  _print_chain(logfile, c, "***   |", "***   `", "***    ", 3, 3);
}

/* ------------------------------------------------------------------------ */

static void check_error_exit(ccmallocLinks * links)
{
  if(continue_flag)
    {
      ccmallocMagicAndFlags * mf = (ccmallocMagicAndFlags*)
        (((char*)links) + ccmallocLinks_ccmallocMagicAndFlags_offset);
     
      set_is_corrupted(mf);
      set_magic(mf);
    }
  else die();
}

/* ------------------------------------------------------------------------ */

static void _check_for_overwrite(ccmallocLinks * links)
{
  char * user_data = ((char*)links) + ccmallocLinks_ccmallocUserData_offset;

  ccmallocBoundary * b =
    (ccmallocBoundary*)(user_data + get_size_links(links));

  char * end_of_user_data = (char*) b;
  int ok = 1;

  align(b);
  while(end_of_user_data < (char*)b)
   {
     ok = (*end_of_user_data ==
       (char) (((unsigned)end_of_user_data) ^ MAGIC_CHAR));

     if(ok) end_of_user_data++;
     else break;
   };

  if(ok)
    {
      int i=0;

      while(i<check_overwrites)
        {
          ok = ((b -> boundary[i] ^ BASIC_SPELL) == (unsigned) user_data);
	  if(ok) i++;
	  else break;
	}
      
      if(!ok)
        {
          fprintf(
	    logfile,
  "*** check-count=%ld: end of block at 0x%08x of size %d changed\n"
  "*** the %d. word (0x%08x) after the block changed\n",
            check_count,
	    (unsigned) user_data,
	    get_size_links(links),
	    i+1,
	    (unsigned) & b -> boundary[i]);

	  fflush(logfile);	/* sometimes print_callchain crashes ... */

          if(!only_count)
	    {
	      fputs(
  "*** it was allocated at\n", logfile);
	      print_callchain(links -> callchain);
	    }
	
	  check_error_exit(links);
	}
    }
  else
    {
      fprintf(
        logfile,
  "*** check-count=%ld: end of block at 0x%08x of size %d changed\n"
  "*** the %d. byte (0x%08x) in the word where the block ends changed\n",
        check_count,
	(unsigned) user_data,
	get_size_links(links),
	(end_of_user_data - user_data) - get_size_links(links) + 1,
	(unsigned) end_of_user_data);

      fflush(logfile);		/* sometimes print_callchain crashes ... */

      if(!only_count)
	{
	  fputs("*** it was allocated at\n", logfile);
	  print_callchain(links -> callchain);
	}
	
      check_error_exit(links);
    }
}

/* ------------------------------------------------------------------------ */

static void _check_for_underwrite(ccmallocLinks * links)
{
  char * user_data = ((char*)links) + ccmallocLinks_ccmallocUserData_offset;
  ccmallocBoundary * b = (ccmallocBoundary*)
    (user_data - sizeof(unsigned) * check_underwrites);

  int ok = 1, i=check_underwrites-1;

  while(i>=0)
    {
      ok = ((b -> boundary[i] ^ BASIC_SPELL) == (unsigned) user_data);
      if(ok) i--;
      else break;
    }

  if(!ok)
    {
      fprintf(
        logfile,
  "*** check-count=%ld: start of block at 0x%08x of size %d changed\n"
  "*** the %d. word (0x%08x) before the block changed\n",
        check_count,
	(unsigned) user_data,
	get_size_links(links),
	check_underwrites-1  - i              + 1,

     /* start                - current_value  + 1 (only CS people start
      *                                            counting from zero ;-)
      */

        (unsigned) & b -> boundary[i]);

      fflush(logfile);		/* sometimes print_callchain crashes ... */

      if(!only_count)
        {
          fputs(
  "*** it was allocated at\n", logfile);
          print_callchain(links -> callchain);
        }
    
      check_error_exit(links);
    }
}

/* ------------------------------------------------------------------------ */

static void _check_for_free_space_integrity(ccmallocLinks * links)
{
  int i, sz = get_size_links(links); 
  char * user_data = ((char*)links) + ccmallocLinks_ccmallocUserData_offset;

  for(i=0; i<sz; i++)
    {
      if(user_data[i] != MAGIC_CHAR)
        {
          ccmallocKeepFreeData * keep_free_data = (ccmallocKeepFreeData*)
              (((char*)links) - ccmallocKeepFreeData_ccmallocLinks_offset);
          
          fprintf(
	    logfile,
  "*** check-count=%ld: byte at 0x%08x changed but lies\n",
            check_count,
	    (unsigned) (user_data + i));

          fprintf(
	     logfile,
  "*** in block at 0x%08x of size %d that was allocated at\n",
            (unsigned) user_data, sz);

          fflush(logfile);	/* sometimes print_callchain crashes ... */

          print_callchain(links -> callchain);

          fflush(logfile);	/* sometimes print_callchain crashes ... */

          fputs(
  "*** and was already freed at\n", logfile);
          print_callchain(keep_free_data -> freed_at);

          check_error_exit(links);
        }
    }
}

/* ------------------------------------------------------------------------ */

/* this is a very expensive routine especially if keep-deallocated-data
 * is set to 1
 */

static void _check_for_integrity()
{
  CCToplevelData * p;

  checks_done++;

  for(p=toplevels; p; p=p->next)
    {
      ccmallocLinks * links;

      for(links=p->allocations; links; links=links->next)
        {
	  if(continue_flag && !is_corrupted_links(links))
	    {
	      if(is_allocated_links(links))
	        {
	          if(check_overwrites) _check_for_overwrite(links);
	          if(check_underwrites) _check_for_underwrite(links);
	        }
	      else
	        {
	          if(check_free_space) _check_for_free_space_integrity(links);
	        }
	    }
	}
    }
}

/* ------------------------------------------------------------------------ */

static void check_for_integrity()
{
  if(!only_count &&
     (check_free_space || check_overwrites) &&
     check_interval > 0)
    {
      if(check_count >= (long)check_start)
        {
	  if(check_modulo == 0) _check_for_integrity();
          check_modulo = (check_modulo + 1) % check_interval;
	}
    }

  check_count++;
}

/* ------------------------------------------------------------------------ */

void ccmalloc_report();

#if defined(sun) && !defined(__svr4__)
int on_exit();
#endif

/* ------------------------------------------------------------------------ */

static void ccmalloc_init()
{
  if(state == UNINITIALIZED)
    {
      state = INITIALIZING;

      libcwrapper_inc_semaphore();

      read_startup_file();	     /* make sure `malloc' is called */
      			             /* (see comment to banner() below */
      {
        ccmallocMagicAndFlags_ccmallocUserData_offset =
	  sizeof(ccmallocMagicAndFlags) + 
	  sizeof(unsigned) * check_underwrites;

	align(ccmallocMagicAndFlags_ccmallocUserData_offset);

	ccmallocSize_ccmallocMagicAndFlags_offset = sizeof(ccmallocSize);
	align(ccmallocSize_ccmallocMagicAndFlags_offset);

	ccmallocSize_ccmallocUserData_offset = 
	  ccmallocSize_ccmallocMagicAndFlags_offset +
	  ccmallocMagicAndFlags_ccmallocUserData_offset;

	assert(isAligned(ccmallocSize_ccmallocUserData_offset));

        ccmallocLinks_ccmallocSize_offset  = sizeof(ccmallocLinks);
	align(ccmallocLinks_ccmallocSize_offset);

	ccmallocLinks_ccmallocMagicAndFlags_offset =
	  ccmallocLinks_ccmallocSize_offset +
	  ccmallocSize_ccmallocMagicAndFlags_offset;

	assert(isAligned(ccmallocLinks_ccmallocMagicAndFlags_offset));

	ccmallocLinks_ccmallocUserData_offset =
	  ccmallocLinks_ccmallocMagicAndFlags_offset +
	  ccmallocMagicAndFlags_ccmallocUserData_offset;

	assert(isAligned(ccmallocLinks_ccmallocUserData_offset));

	ccmallocKeepFreeData_ccmallocLinks_offset = sizeof(ccmallocLinks);
	align(ccmallocKeepFreeData_ccmallocLinks_offset);
      }

      if(only_count)
        {
	  offset = ccmallocSize_ccmallocUserData_offset;
	}
      else
        {
          init_CallChain();

	  offset = ccmallocLinks_ccmallocUserData_offset;

	  if(keep_deallocated_data)
	    offset += ccmallocKeepFreeData_ccmallocLinks_offset;
	  
	  assert(isAligned(offset));
	}
      
      /* USE_DTOR_FOR_REPORTING is used for C++ (ccmalloc.o)
       * and reporting is done after ccmalloc_report is 
       * called the second time!
       */

#     ifdef sun
#       ifdef __svr4__
          atexit(ccmalloc_report);		/* solaris */
#       else
          on_exit(ccmalloc_report);		/* sunos4.3.1 ? */
#       endif
#     else
        atexit(ccmalloc_report);		/* linux and others */
#     endif
      
      if(!silent_startup) banner();  /* now malloc has already been called */
      				     /* and any underlying memory profiler */
				     /* has reported its banner */

      libcwrapper_dec_semaphore();
      state = INITIALIZED;
    }
}

/* ------------------------------------------------------------------------ */

static void * _ccmalloc_malloc(size_t number_of_bytes)
{
  char * data, * user_data;
  ccmallocMagicAndFlags * mf;
  ccmallocSize * sz;
  size_t n;

  check_for_integrity();

  if(number_of_bytes < 1)	/* works for signed *and* unsigned size_t */
    {
      callchain_msg("*** malloc(%d)\n", number_of_bytes);
      if(continue_flag) return 0; else die();
    }

  n = offset + number_of_bytes;

  if(check_overwrites)
    {
      align(n);
      n += sizeof(unsigned) * check_overwrites;
    }

  data = (char*) malloc(n);
  data += offset;
  user_data = data;

  if(check_underwrites)
    {
      ccmallocBoundary * b = (ccmallocBoundary*)
        (user_data - sizeof(unsigned) * check_underwrites);

      int i;

      for(i=0; i<check_underwrites; i++)
        b -> boundary[i] = BASIC_SPELL ^ ((unsigned) user_data);
    }

  /* Now we initialize all components of the header in reverse order.
   */

  data -= ccmallocMagicAndFlags_ccmallocUserData_offset;
  mf = (ccmallocMagicAndFlags *) data;

  sz = (ccmallocSize *) (data - ccmallocSize_ccmallocMagicAndFlags_offset);
  sz -> size = number_of_bytes;

  set_is_allocated(mf);
  set_magic(mf);		/* may depend on `sz -> size' */

  if(check_overwrites)
    {
      int i;
      ccmallocBoundary * b =
        (ccmallocBoundary*) (user_data + number_of_bytes);
      char * end_of_user_data = (char*) b;

      align(b);
      while(end_of_user_data<(char*)b)
        {
          *end_of_user_data =
            (char) (((unsigned)end_of_user_data) ^ MAGIC_CHAR);
          end_of_user_data++;
        };

      for(i=0; i<check_overwrites; i++)
        b -> boundary[i] = BASIC_SPELL ^ ((unsigned)user_data);
    }

  if(!only_count)
    {
      CallChain c;
      ccmallocLinks * links;

      data -= ccmallocLinks_ccmallocMagicAndFlags_offset;
      links = (ccmallocLinks*) data;

      if(!(c=backtrace(2))) c = empty_call_chain;
      enqueue(c, links);

      if(callchain_statistics)
        {
          CallChainStats * stats;

	  stats = c -> toplevel_data -> stats;
          stats -> num_allocated++;
          stats -> bytes_allocated += number_of_bytes;
        }
      
      if(keep_deallocated_data)
        {
          /* it's defensive to initialize freed_at */

          ccmallocKeepFreeData * fd;
          data -= ccmallocKeepFreeData_ccmallocLinks_offset;
          fd = (ccmallocKeepFreeData*) data;
          fd -> freed_at = 0;
        }
    }
 
  return user_data;
}

/* ------------------------------------------------------------------------ */

static void _ccmalloc_free(void * _data)
{
  char * data, * user_data;
  ccmallocMagicAndFlags * mf;

  check_for_integrity();

  if(!_data)
    {
      callchain_msg("*** can not apply free to zero pointer\n");
      if(continue_flag) return; else die();
    }

  data = user_data = (char*) _data;
  data -= ccmallocMagicAndFlags_ccmallocUserData_offset;
  mf = (ccmallocMagicAndFlags*) data;

  if(is_valid_magic(mf))
    {
      int i;
      ccmallocSize * sz;

      if(!is_allocated(mf))
        {
          ccmallocLinks * links = (ccmallocLinks*)
	    (user_data - ccmallocLinks_ccmallocUserData_offset);

          fprintf(logfile,
                  "*** free() called twice for block at 0x%08x of size %d\n",
                  (unsigned) user_data, get_size_links(links));

	  fflush(logfile);	/* print_callchain crashes sometimes */
          
          if(!only_count)
            {
              fputs("*** it was allocated at\n", logfile);

              print_callchain(links -> callchain);
	      fflush(logfile);
          
              if(keep_deallocated_data)
                {
                  ccmallocKeepFreeData * keep_free_data =
                    (ccmallocKeepFreeData*)
                      (((char*)links) -
                               ccmallocKeepFreeData_ccmallocLinks_offset);

                  fputs("*** and already freed at\n", logfile);
                  print_callchain(keep_free_data -> freed_at);
                }
            }

          if(continue_flag) return; else die();
        }

      data -= ccmallocSize_ccmallocMagicAndFlags_offset;
      sz = (ccmallocSize*) data;

      if(check_underwrites)
        {
	  ccmallocLinks * links = (ccmallocLinks*)
	    (user_data - ccmallocLinks_ccmallocUserData_offset);
	  
	  if(!is_corrupted(mf)) _check_for_underwrite(links);
	}

      if(check_overwrites)
        {
	  ccmallocLinks * links = (ccmallocLinks*)
	    (user_data - ccmallocLinks_ccmallocUserData_offset);
	  
	  if(!is_corrupted(mf)) _check_for_overwrite(links);
	}

      bytes_deallocated += sz -> size;
      num_deallocated++;

      if(only_count)
	{
	  assert(data + offset == user_data);
	}
      else
	{
	  ccmallocLinks * links;

	  data -= ccmallocLinks_ccmallocSize_offset;
	  links = (ccmallocLinks*) data;

	  if(callchain_statistics)
	    {
	      CallChainStats * stats;

	      stats = links -> callchain -> toplevel_data -> stats;
	      stats -> num_deallocated++;
	      stats -> bytes_deallocated += sz -> size;
	    }

	  if(keep_deallocated_data)
	    {
	      CallChain c;
	      ccmallocKeepFreeData * keep_free_data;
	      data -= ccmallocKeepFreeData_ccmallocLinks_offset;
	      keep_free_data = (ccmallocKeepFreeData*) data;

	      if(!(c = backtrace(2))) c = empty_call_chain;
	      keep_free_data -> freed_at = c;
	    }
	  else dequeue(links);
	}

      /* Try to invalidate deallocated data! */

      for(i=0; i<sz -> size; i++) user_data[i] = MAGIC_CHAR;

      if(keep_deallocated_data)
	{
	  clr_is_allocated(mf);
	  set_magic(mf);
	}
      else
	{
	  /* Here we should invalidate the magic because the underlying
	   * memory allocator may corrupt the ccmalloc data like
	   * the pointer to the callchain.
	   */

	  invalidate_magic(mf);
	  free(data);
	}
    }
  else
    {
      fprintf(logfile,
              "*** can not free non valid data at 0x%08x\n",
              (unsigned)user_data);


      if(keep_deallocated_data)
	fputs("*** (perhaps an `under'write occured)\n", logfile);
      else
        {
          fputs(
"*** (Perhaps an `under'write occured or data has already been freed\n"
"***  You should enable `keep-deallocated-data' to test the last case!)\n",
            logfile);
	}

      if(continue_flag) return; else die();
    }
}

/* ------------------------------------------------------------------------ */

static void _ccmalloc_report()
{
  libcwrapper_inc_semaphore();

  fprintf(logfile,
"\n"
".---------------.\n"
"|ccmalloc report|\n"
"=======================================================\n"
"| total # of|   allocated | deallocated |     garbage |\n"
"+-----------+-------------+-------------+-------------+\n"
"|      bytes|%12ld |%12ld |%12ld |\n"
"+-----------+-------------+-------------+-------------+\n"
"|allocations|%12ld |%12ld |%12ld |\n",
	  bytes_allocated, bytes_deallocated,
	  bytes_allocated - bytes_deallocated,
	  num_allocated, num_deallocated,
	  num_allocated - num_deallocated);
  
  if(!only_count)
    {
      fputs(
"+-----------------------------------------------------+\n", logfile);

      _check_for_integrity();
      fprintf(logfile,
              "| number of checks: %-34ld|\n"
	      "| number of counts: %-34ld|\n",
	      checks_done, check_count);

      copy_chaintab_into_chains();
      name_all_chains();

      assert(file_name);

      insert_file_info_in_chains(file_name);
      sort_chains();
      report_callchain_counts();
      report_callchains();
    }

  fputs(
"`------------------------------------------------------\n", logfile);

  fflush(logfile);			/* defensive ... */
  exit_CallChain();			/* cleanup */

  libcwrapper_dec_semaphore();
}

/*---------------------------------------------------------------------------.
 | Here starts the part of external functions                                |
 `---------------------------------------------------------------------------*/

void ccmalloc_report()
{
  switch(state)
    {
      case INITIALIZED:
         
#if USE_DTOR_FOR_REPORTING

	state = TRIED_TO_REPORT_ONCE;
	break;

#endif	/* !!!!!!!!!!!!! else fall through !!!!!!!!!!!!!!!! */

      case TRIED_TO_REPORT_ONCE:

        _ccmalloc_report();
	state = FINISHED;
	break;
      
      default:

        callchain_error("ccmalloc_report() called in non valid state\n");
	break;
    }
}

/* ------------------------------------------------------------------------ */

void * ccmalloc_malloc(size_t number_of_bytes)
{
  void * res;

  switch(state)
    {
      default:
        callchain_fatal("non valid state in malloc\n");

	/* will never be here */
        return 0;

      case FINISHED:

        res = malloc(number_of_bytes);
	break;

      case UNINITIALIZED:

        ccmalloc_init();

	/*!!!!!!!! no break -> fall through !!!!!!!!!*/

      case INITIALIZED:
      case TRIED_TO_REPORT_ONCE:	/* ifdef USE_DTOR_FOR_REPORTING */

	res = _ccmalloc_malloc(number_of_bytes);
	break;
    }

  if(number_of_bytes>0)		/* ok if size_t is unsigned *or* signed */
    {
      bytes_allocated += number_of_bytes;
      num_allocated++;
    }
  
  return res;
}

/* ------------------------------------------------------------------------ */

void ccmalloc_free(void * _data)
{
  switch(state)
    {
      case UNINITIALIZED:

        callchain_fatal(
	  "free() without any malloc in uninitialized state\n");
	assert(0);
	exit(1);
	break;
      
      default:

        callchain_fatal("non valid state in free()\n");
        break;
      
      case FINISHED:

	callchain_msg("free(0x%08x) after reporting\n", (unsigned) _data);
	if(tell_about_dtors)
	  {
callchain_msg("  (This can happen with static destructors.\n");

#           if defined(USE_DTOR_FOR_REPORTING)
              {
callchain_msg("   When linking put `ccmalloc.o' at the end (for gcc) or\n");
callchain_msg("   in front of the list of object files.)\n");
              }
#           else
              {
callchain_msg("   Link with ccmalloc.o instead of -lccmalloc!)\n");
	      }
#	    endif

	    tell_about_dtors = 0;
	  }
	break;

      case INITIALIZED:
      case TRIED_TO_REPORT_ONCE:	/* ifdef USE_DTOR_FOR_REPORTING */

	_ccmalloc_free(_data);
	break;
    }
}

/* ------------------------------------------------------------------------ */

int
ccmalloc_size(void * _data, size_t * size_ptr)
{
  char * data;
  ccmallocMagicAndFlags * mf;  

  if(!_data)
    {
      fprintf(logfile, "*** internal error in ccmalloc_size\n");
      die();
    }
  
  data = (char *) _data;
  data -= ccmallocMagicAndFlags_ccmallocUserData_offset;
  mf = (ccmallocMagicAndFlags*) data;

  if(is_valid_magic(mf))
    {
      ccmallocSize * sz;
      data -= ccmallocSize_ccmallocMagicAndFlags_offset;
      sz = (ccmallocSize *) data;
      *size_ptr = sz -> size;
      return 1;
    }
  else return 0;
}

/* ------------------------------------------------------------------------ */

void ccmalloc_die_or_continue(char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(logfile, fmt, ap);
  va_end(ap);
  if(!continue_flag) die();
}

/* ------------------------------------------------------------------------ */
/* give the user a chance to force a check at will */

extern void ccmalloc_check_for_integrity()
{
  libcwrapper_inc_semaphore();
  check_count++;
  _check_for_integrity();
  libcwrapper_dec_semaphore();
}

/* ------------------------------------------------------------------------ */

/*----------------------------------------------------------------------------
 * (C) 1997-1998 Armin Biere 
 *
 *     $Id: hash.c,v 1.1.1.1 2001/11/30 07:54:36 shaleh Exp $
 *----------------------------------------------------------------------------
 */

#include "hash.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

/* ------------------------------------------------------------------------ */

typedef struct HashTableBucket 
{
  void * data;				/* do not move this !!!! */
  struct HashTableBucket * next;
}
HashTableBucket;

/* ------------------------------------------------------------------------ */

typedef struct HashTable_
{
  int count;
  int size;
  int threshold;
  int num_resizes;
  int primes_idx;

  HashTableBucket ** table;

  int (*cmp_keys)(void * key1, void * key2);
  int (*hash_key)(void * key);
  void * (*get_key_from_data)(void * data);
  void (*free_data)(void *);
  void (*error)(char *);
}
HashTable_;

/* ------------------------------------------------------------------------ */

/* let s be the current size and s' the desired next size
 * we request a larger size iff n = 80% s where n is
 * the number of valid nodes. Now we want to have
 * n = 50% s' for not wasting too much space. 
 * This gives the formula
 *
 *    s' = (8/5) s
 *
 *  the primes can be generated with the following awk program:
 *
 *    BEGIN {
 *      P = 3; M = 2147483647	# 2**31
 *      while(P<M) {
 *        print P ",";
 *        P = int(8*P/5); "primes " P | getline; P = $1
 *      }
 *    }
 */

static int primes[] =
{
3,		7,		13,		31,		61,
127,		251,		509,		1021,		2039,
4093,		8191,		16381,		26209,		41941,
67121,		107441,		171917,		275083,		440159,
704269,		1126831,	1802989,	2884787,	4615661,
7385057,	11816111,	18905813,	30249367,	48399019,
77438437,	123901501,	198242441,	317187907,	507500657,
812001067,	1299201731,	2078722769
};

/* ------------------------------------------------------------------------ */

static int simple_cmp_keys(void * k1, void * k2)
{
  if(k1 == k2) return 0;
  else if(k1 < k2) return 1;
  else return -1;
}

/* ------------------------------------------------------------------------ */

static int simple_hash_key(void * key) { return (int) key; }

/* ------------------------------------------------------------------------ */

static void * simple_get_key_from_data(void * a) { return a; }

/* ------------------------------------------------------------------------ */

static void simple_free_data(void * data) { (void) data; }

/* ------------------------------------------------------------------------ */

static void simple_error(char * errmsg)
{
  fprintf(stderr, "*** in hash.c: %s\n", errmsg);
  fflush(stderr);
  kill(0, SIGSEGV);
}

/* ------------------------------------------------------------------------ */

#define setToSimpleIfZero(a) a ? a : simple_ ## a

/* ------------------------------------------------------------------------ */

HashTable 
new_HashTable(
  int minimal_size,
  int (*cmp_keys)(void * key1, void * key2),
  int (*hash_key)(void * key),
  void* (*get_key_from_data)(void * data),
  void (*free_data)(void*),
  void (*error)(char *))
{
  HashTable res = (HashTable) malloc(sizeof(HashTable_));
  int primes_idx = 0, i;

  while(minimal_size > primes[primes_idx])
    primes_idx++;

  res -> primes_idx = primes_idx;
  res -> size = primes[primes_idx];
  res -> count = 0;

  res -> cmp_keys = setToSimpleIfZero(cmp_keys);
  res -> hash_key = setToSimpleIfZero(hash_key);
  res -> get_key_from_data = setToSimpleIfZero(get_key_from_data);
  res -> free_data = setToSimpleIfZero(free_data);
  res -> error = setToSimpleIfZero(error);

  /* ceil(0.8 * res -> size) */

  res -> threshold = (((res -> size) << 2) + 4) / 5;
  res -> num_resizes = 0;

  res -> table = (HashTableBucket **)
                 malloc(res -> size * sizeof(HashTableBucket*));
  
  for(i=0; i<res->size; i++)
    res -> table[i] = 0;

  return res;
}

/* ------------------------------------------------------------------------ */

void
free_HashTable(HashTable ht)
{
  int i;
  void (*free_data)(void *);
  free_data = ht -> free_data;

  for(i=0; i<ht->size; i++)
    {
      HashTableBucket * p, * tmp;
      for(p=ht->table[i]; p; p = tmp)
        {
	  tmp = p -> next;
	  free_data(p -> data);
	  free(p);
	}
    }
  
  free(ht -> table);
  free(ht);
}

/* ------------------------------------------------------------------------ */

static void
resize_HashTable(HashTable ht)
{
  HashTable new_ht = new_HashTable(primes[ht -> primes_idx + 1],
		                   ht -> cmp_keys,
		                   ht -> hash_key,
		                   ht -> get_key_from_data,
		                   ht -> free_data,
				   ht -> error);
  int i;
  void* (*get_key_from_data)(void*) = ht -> get_key_from_data;

  for(i=0; i<ht->size; i++)
    {
      HashTableBucket * p, * tmp;
      for(p=ht -> table[i]; p; p = tmp)
        {
	  HashTableBucket ** position =
	    (HashTableBucket**) get_position_of_key_in_HashTable(
			          new_ht, get_key_from_data(p -> data));
	  tmp = p -> next;
	  p -> next =  *position;
	  *position = p;
	  new_ht -> count ++;
	}
    }

  free(ht -> table);
  new_ht -> num_resizes = ht -> num_resizes + 1;
  *ht = *new_ht;
  free(new_ht);
}

/* ------------------------------------------------------------------------ */

void **
get_position_of_key_in_HashTable(HashTable ht, void * key)
{
  int h = ht -> hash_key(key);
  HashTableBucket **p;
  int (*cmp_keys)(void *, void *) = ht -> cmp_keys;
  void* (*get_key_from_data)(void *) = ht -> get_key_from_data;

  if(h<0) h=-h;
  h %= ht -> size;
  p = &(ht -> table[h]);

  while(*p)
    {
      if(cmp_keys(key, get_key_from_data((*p) -> data)) == 0)
        {
          return (void**)p;
	}
      else p  = &((*p) -> next);
    }

  return (void**)p;
}

/* ------------------------------------------------------------------------ */

void *
get_data_from_position_in_HashTable(HashTable ht, void ** position)
{
  (void) ht;
  return (* (HashTableBucket**) position) -> data;
}

/* ------------------------------------------------------------------------ */

int
get_size_of_HashTable(HashTable ht)
{
  return ht -> count;		/* do not mess up with ht -> size ! */
}

/* ------------------------------------------------------------------------ */

const char *
get_statistics_for_HashTable(HashTable ht)
{
  static char buffer[200];
  int collisions = 0, maxchain = 0, i;

  for(i=0; i<ht->size; i++)
    {
      HashTableBucket * b = ht -> table[i];
      if(b)
        {
	  int j = 1;
	  for(b = b -> next; b; b = b -> next)
	    j++;
	  
	  if(maxchain<j) maxchain = j;
	  if(j>1) collisions += j - 1;
	}
    }

  sprintf(buffer,
          "elements=%d size=%d resizes=%d collisions=%d maxchain=%d",
	  ht -> count, ht -> size, ht -> num_resizes, collisions, maxchain);
  
  return buffer;
}

/* ------------------------------------------------------------------------ */

void
insert_at_position_into_HashTable(
  HashTable ht,
  void ** _position,
  void * data)
{
  HashTableBucket * b, ** position = (HashTableBucket**) _position;

  if(!position)
    (ht -> error)("insert_at_position_into_HashTable: non valid position");

  if(ht -> count >= ht -> threshold)
    {
      resize_HashTable(ht);
      position = (HashTableBucket**)
        get_position_of_key_in_HashTable(ht, (ht -> get_key_from_data)(data));
    }

  b = (HashTableBucket*) malloc(sizeof(HashTableBucket));
  b -> data = data;
  b -> next = *position;
  *position = b;
  ht -> count++;
}

/* ------------------------------------------------------------------------ */

void
overwrite_at_position_in_HashTable(
  HashTable ht, void * _position, void * data)
{
  HashTableBucket ** position = (HashTableBucket**) _position, *tmp;

  if(!position)
    (ht -> error)("overwrite_at_position_in_HashTable: non valid position");

  tmp = *position;

# ifdef DEBUG
    {
      void * keyOld = (ht -> get_key_from_data)(tmp -> data);
      void * keyNew = (ht -> get_key_from_data)(data);

      if((ht -> cmp_keys)(keyOld, keyNew) != 0)
        (ht -> error)
	  ("overwrite_at_position_in_HashTable: keys do not match");
    }
# endif

  (ht -> free_data)(tmp -> data);
  tmp -> data = data;
}

/* ------------------------------------------------------------------------ */

int
insert_into_HashTable(HashTable ht, void * data)
{
  void ** position =
    get_position_of_key_in_HashTable(ht, (ht -> get_key_from_data)(data));
  
  if(!*position)
    {
      insert_at_position_into_HashTable(ht, position, data);
      return 0;
    }
  else return 1;
}

/* ------------------------------------------------------------------------ */

void
overwrite_in_HashTable(HashTable ht, void * data)
{
  void ** position =
    get_position_of_key_in_HashTable(ht, (ht -> get_key_from_data)(data));
  
  if(*position)
    {
      insert_at_position_into_HashTable(ht, position, data);
    }
  else
    {
      overwrite_at_position_in_HashTable(ht, position, data);
    }
}

/* ------------------------------------------------------------------------ */

void *
get_data_from_HashTable(HashTable ht, void * key)
{
  HashTableBucket * bucket =
    *(HashTableBucket**) get_position_of_key_in_HashTable(ht, key);

  return bucket ? bucket -> data : 0;
}

/* ------------------------------------------------------------------------ */

void
remove_from_HashTable(HashTable ht, void * key)
{
  HashTableBucket * tmp, ** position =
    (HashTableBucket**) get_position_of_key_in_HashTable(ht, key);
  
  if((tmp = *position))
    {
      *position = tmp -> next;
      ht -> free_data(tmp -> data);
      free(tmp);
    }
}

/* ------------------------------------------------------------------------ */

void
setup_HashTableIterator(HashTable ht, HashTableIterator * iterator)
{
  iterator -> ht = ht;
  iterator -> pos = -1;	/* have a look at increment_HashTableIterator */
  iterator -> cursor = 0;
  increment_HashTableIterator(iterator);
}

/* ------------------------------------------------------------------------ */

void
increment_HashTableIterator(HashTableIterator * iterator)
{
  HashTableBucket * current = (HashTableBucket*) iterator -> cursor;

  if(current && current -> next)
    {
      iterator -> cursor = current -> next;
    }
  else
    {
      int i = iterator -> pos + 1;
      int sz = iterator -> ht -> size;
      HashTableBucket ** table = iterator -> ht -> table;

      while(i < sz)
        {
	  if(table[i])
	    {
	      iterator -> pos = i;
	      iterator -> cursor = table[i];
	      return;
	    }
	  else i++;
	}
      
      iterator -> cursor = 0;
      iterator -> pos = sz;
    }
}

/* ------------------------------------------------------------------------ */

int is_done_HashTableIterator(HashTableIterator * iterator)
{
  return iterator -> cursor == 0;
}

/* ------------------------------------------------------------------------ */

void *
get_data_from_HashTableIterator(HashTableIterator * iterator)
{
  HashTableBucket * bucket = (HashTableBucket*) iterator -> cursor; 

  if(!bucket) return 0;
  else return bucket -> data;
}

/* ------------------------------------------------------------------------ */

/*---------------------------------------------------------------------------.
 | the following function is the famous hash function of P.J. Weinberger as  |
 | desribed in the dragon book of A.V. Aho, R. Sethi and J.D. Ullman.        |
 `---------------------------------------------------------------------------*/

#define HASHPJWSHIFT ((sizeof(int))*8-4)

int 
hashpjw_HashTable(void * _s)
{
  const char * s = (char*) _s, * p;
  int h=0, g;

  for(p=s; *p!='\0'; p++)
    {
      h = (h<<4) + *p;
      if( (g = h & (0xf<<HASHPJWSHIFT)) )
        {
	  h = h^(g>>HASHPJWSHIFT);
	  h = h^g;
        }
    }

  return h > 0 ? h : -h; // no modulo but positive
}

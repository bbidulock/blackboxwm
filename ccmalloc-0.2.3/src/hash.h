/*----------------------------------------------------------------------------
 * (C) 1997-1998 Armin Biere 
 *
 *     $Id: hash.h,v 1.1.1.1 2001/11/30 07:54:36 shaleh Exp $
 *----------------------------------------------------------------------------
 */
#ifndef _hash_h_INCLUDED
#define _hash_h_INCLUDED

/*---------------------------------------------------------------------------.
 | This is an implementation of a general hash table module.                 |
 | It assumes that keys for data stored in the hash table can                |
 | be extracted from the data.                                               |
 |                                                                           |
 | One very important feature of this implementation is that                 |
 | the data stored in the hashtable is `owned' by the hash                   |
 | table. The user should not delete it! It will be deleted                  |
 | by the destructor of the hash table or by an `overwrite'                  |
 | method. To ease the use of different allocators and to                    |
 | be able to store statically allocated data in a hash table                |
 | the user can provide his own `free' function.                             |
 |                                                                           |
 | TOF                                                                       |
 |                                                                           |
 | HashTable:                                                                |
 |                                                                           |
 |   new_HashTable                         constructor                       |
 |   free_HashTable                        destructor                        |
 |                                                                           |
 |   insert_into_HashTable                 maninpulator                      |
 |   insert_at_position_into_HashTable     maninpulator                      |
 |   overwrite_in_HashTable                maninpulator                      |
 |   overwrite_at_position_in_HashTable    maninpulator                      |
 |   remove_from_HashTable                 maninpulator                      |
 |                                                                           |
 |   get_data_from_HashTable               inspector                         |
 |   get_data_from_position_in_HashTable   inspector                         |
 |   get_position_of_key_in_HashTable      inspector                         |
 |                                                                           |
 |   get_size_of_HashTable                 inspector                         |
 |   get_statistics_of_HashTable           inspector                         |
 |                                                                           |
 | HashTableIterator:                                                        |
 |                                                                           |
 |                                         no constructors or                |
 |                                         destructors                       |
 |                                                                           |
 |   setup_HashTableIterator               maninpulator                      |
 |   increment_HashTableIterator           maninpulator                      |
 |                                                                           |
 |   is_done_HashTableIterator             inspector                         |
 |   get_data_from_HashTableIterator       inspector                         |
 |                                                                           |
 | general:                                                                  |
 |                                                                           |
 |   hashpjw_HashTable                                                       |
 `---------------------------------------------------------------------------*/

typedef struct HashTable_ * HashTable;

/* Construct a new HashTable. The first parameter gives the initial
 * minimal size of the hashtable. The hashtable resizes itself if
 * necessary and grows exponentially with each resize step.
 * This means that using 0 as minimal_size does not really hurt too much.
 *
 * The next five function parameters are used to control the
 * data and the keys stored in the hash table.
 *
 *   cmp_keys  -- is the comparison function for keys. It should
 *                return the same results as the `strcmp'function.
 *                (actually: 0 iff the two args are the same)
 *                If cmp_keys is 0 then pointer comparison is used.
 *
 *   hash_key  -- hash function (see also hashpjw_HashTable). If it
 *                is zero than a cast from `void *' to `int' is used.
 * 
 *   get_key_from_data --
 *                this function is used to get the key from
 *                the data stored in the hash table. If this parameter
 *                is zero than it is assumed that the data is the
 *                also its key.
 *
 *   free_data -- user provided deallocator of data stored in the
 *                hashtable. If zero do not deallocate data.
 * 
 *   error     -- this function is called if an internal error
 *                ocurred (f.e. interface violation). As default
 *                (set to zero) print an descriptive error message
 *                and produce a segmentation fault.
 */

extern HashTable 
new_HashTable(
  int minimal_size,
  int   (*cmp_keys)(void * key1, void * key2),
  int   (*hash_key)(void * key),
  void* (*get_key_from_data)(void * data),
  void  (*free_data)(void * data),
  void  (*error)(char * errmsg));

/* Destructor of HashTables. Free all stored data with the user provided
 * deallocation function.
 */

extern void
free_HashTable(HashTable);

/* insert a data into the HashTable but do not overwrite existing
 * data with the same key. The return value is true iff
 * some data with the same key is already stored in the hash table.
 * In this case the user generally has to deallocate `data_with_key'
 * on his own.
 */

extern int
insert_into_HashTable(HashTable ht, void * data_with_key);

/* remove correponding data from the hash table and deallocate
 * it using the user provided deallocator.
 */

extern void
remove_from_HashTable(HashTable ht, void * key);

/* overwrite an existing entry in the hash table with the same
 * key as the key of `data_with_key' or insert a new entry.
 * In the first case also deallocate the overwritten data.
 */

extern void
overwrite_in_HashTable(HashTable ht, void * data_with_key);

/* search for data with this key. Return zero if not found.
 * (see also `get_data_from_position_in_HashTable')
 */

extern void *
get_data_from_HashTable(HashTable ht, void * key);

/* return the number of stored key/data pairs
 */

extern int
get_size_of_HashTable(HashTable ht);

/* Returns an statically allocated string with useful ;-)
 * statistical information. This string is overwritten by
 * subsequent calls to this function.
 */

extern const char *
get_statistics_for_HashTable(HashTable ht);

/* For many applications of hash tables (f.e. a symbol table
 * of a compiler) the user must check if some data with a
 * given key already exists in the hash table before he
 * actually constructs the data (f.e. a new entry into the
 * symbol table). If no data with this key is found then
 * the new allocated data is inserted into the hash table.
 * This means that the same search must be done again.
 *
 * This can be avoided if we know from the first search
 * where the new allocated data will be inserted. For
 * this purpose `get_position_of_key_in_HashTable' can be used.
 * The *dereferenced* return value is exactly 0 iff the search
 * was succesfull. In this case
 * `get_data_from_position_in_HashTable' can be used to get the
 * actual data for the key. If the search is not succesfull
 * the user can now allocated the new data and insert it
 * at the returned position with 
 * `insert_at_position_into_HashTable' or
 * `overwrite_at_position_in_HashTable'.
 */

extern void **
get_position_of_key_in_HashTable(HashTable ht, void * key);

extern void *
get_data_from_position_in_HashTable(HashTable, void ** position);

extern void
insert_at_position_into_HashTable(
  HashTable ht, void ** position, void * data);

extern void
overwrite_at_position_in_HashTable(
  HashTable ht, void * position, void * data);

/* The definition of this structure is placed here
 * because it should be possible to allocate an iterator as
 * an automatic variable. This is also more convenient for
 * the user.
 */

typedef struct HashTableIterator 
{
  HashTable ht;
  int pos;
  void * cursor;
}
HashTableIterator;

/* Setup an iterator. It is possible to have multiple
 * iterators for the same hash table and to traverse
 * the hash table in parallel.
 * ATTENTION: These iterators are not save with
 * respect to `remove' operations on the traversed
 * hash table.
 */

extern void
setup_HashTableIterator(HashTable, HashTableIterator *);

extern void
increment_HashTableIterator(HashTableIterator *);

extern void *
get_data_from_HashTableIterator(HashTableIterator *);

extern int
is_done_HashTableIterator(HashTableIterator *);

/* Hash function for zero terminated data (f.e. strings).
 * It is from the `dragon book' and should work very
 * well especially for strings as keys.
 */

extern int hashpjw_HashTable(void *);

#endif

/*
Copyright (c) 2020, Johnathan Corkery. (jcorkery@umich.edu)
All rights reserved.

This file is part of the topaz project (https://github.com/jcorks/topaz)
gensyn was released under the MIT License, as detailed below.



Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal 
in the Software without restriction, including without limitation the rights 
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
copies of the Software, and to permit persons to whom the Software is furnished 
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.


*/


#ifndef H_GENSYNDC__TABLE__INCLUDED
#define H_GENSYNDC__TABLE__INCLUDED

/*

    Table
    -----

    Hashtable able to handle various kinds of keys 

    For buffer and string keys, key copies are created, so 
    the source key does not need to be kept in memory
    once created.


*/
typedef struct gensyn_table_t gensyn_table_t;

/// Creates a new table whose keys are C-strings.
///
gensyn_table_t * gensyn_table_create_hash_c_string();

/// Creates a new table while keys are gensyn strings.
///
gensyn_table_t * gensyn_table_create_hash_gensyn_string();


/// Creates a new table whose keys are a byte-buffer of 
/// the specified length.
///
gensyn_table_t * gensyn_table_create_hash_buffer(int n);


/// Creates a new table whose keys are a pointer value.
///
gensyn_table_t * gensyn_table_create_hash_pointer();




/// Frees the given table.
///
void gensyn_table_destroy(gensyn_table_t *);


/// Inserts a new key-value pair into the table.
/// If a key is already within the table, the value 
/// corresponding to that key is updated with the new copy.
///
/// Notes regarding keys: when copied into the table, a value copy 
/// is performed if this hash table's keys are pointer values.
/// If a buffer or string, a new buffer is stored and kept until 
/// key-value removal.
///
void gensyn_table_insert(gensyn_table_t *, const void * key, void * value);

/// Same as gensyn_table_insert, but treats the key as a signed integer
/// Convenient for hash_pointer tables where keys are direct pointers.
/// 
#define gensyn_table_insert_by_int(__T__, __K__, __V__) (gensyn_table_insert(__T__, (void*)(intptr_t)__K__, __V__))

/// Same as gensyn_table_insert, but treats the key as an un signed integer
/// Convenient for hash_pointer tables where keys are direct pointers.
/// 
#define gensyn_table_insert_by_uint(__T__, __K__, __V__) (gensyn_table_insert(__T__, (void*)(uintptr_t)__K__, __V__))


/// Returns the value corresponding to the given key.
/// If none is found, NULL is returned. Note that this 
/// implies useful output only if key-value pair contains 
/// non-null data. You can use "gensyn_table_entry_exists()" to 
/// handle NULL values.
///
void * gensyn_table_find(const gensyn_table_t *, const void * key);

/// Same as gensyn_table_find, but treats the key as a signed integer
/// Convenient for hash_pointer tables where keys are direct pointers.
/// 
#define gensyn_table_find_by_int(__T__, __K__) (gensyn_table_find(__T__, (void*)(intptr_t)(__K__)))

/// Same as gensyn_table_find, but treats the key as an unsignedinteger
/// Convenient for hash_pointer tables where keys are direct pointers.
/// 
#define gensyn_table_find_by_uint(__T__, __K__) (gensyn_table_find(__T__, (void*)(uintptr_t)(__K__)))

/// Returns TRUE if an entry correspodning to the 
/// given key exists and FALSE otherwise.
///
int gensyn_table_entry_exists(const gensyn_table_t *, const void * key);


/// Removes the key-value pair from the table whose key matches 
/// the one given. If no such pair exists, no action is taken.
///
void gensyn_table_remove(gensyn_table_t *, const void * key);

/// Returns whether the table has entries.
///
int gensyn_table_is_empty(const gensyn_table_t *);

/// Removes all key-value pairs.
///
void gensyn_table_clear(gensyn_table_t *);






/*

    TableIter
    ---------

    Helper class for iterating through hash tables


*/

typedef struct gensyn_table_iter_t gensyn_table_iter_t;


/// Creates a new hash table iterator.
/// This iterator can be used with any table, but needs 
/// to be "started" with the table in question.
///
gensyn_table_iter_t * gensyn_table_iter_create();


/// Destroys a table iter.
///
void gensyn_table_iter_destroy(gensyn_table_iter_t *);

/// Begins the iterating process by initializing the iter 
/// to contain the first key-value pair within the table.
///
void gensyn_table_iter_start(gensyn_table_iter_t *, gensyn_table_t *);


/// Goes to the next available key-value pair in the table 
/// 
void gensyn_table_iter_proceed(gensyn_table_iter_t *);

/// Returns whether the end of the table has been reached.
///
int gensyn_table_iter_is_end(const gensyn_table_iter_t *);


/// Returns the key (owned by the table) for the current 
/// key-value pair. If none, returns NULL.
///
const void * gensyn_table_iter_get_key(const gensyn_table_iter_t *);

/// Returns the value for the current 
/// key-value pair. If none, returns NULL.
///
void * gensyn_table_iter_get_value(const gensyn_table_iter_t *);


#endif

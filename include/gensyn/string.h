/*
Copyright (c) 2020, Johnathan Corkery. (jcorkery@umich.edu)
All rights reserved.

This file was originally part of the topaz project (https://github.com/jcorks/topaz)
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


#ifndef H_GENSYNDC__STRING__INCLUDED
#define H_GENSYNDC__STRING__INCLUDED

#include <stdint.h>

/*

    String
    -----
    


*/
typedef struct gensyn_string_t gensyn_string_t;


/// Creates a new, empty string. 
///
gensyn_string_t * gensyn_string_create();

/// Creates a new string initialized with the contents of the given C-string 
///
gensyn_string_t * gensyn_string_create_from_c_str(const char *, ...);

/// Creates a new string as a copy of the given string 
///
gensyn_string_t * gensyn_string_clone(const gensyn_string_t *);

/// Destroys and frees a gensyn string 
///
void gensyn_string_destroy(gensyn_string_t *);





/// Returns a temporary string built from the given cstring 
/// It is meant as a convenience function, but it has the following 
/// restrictions:
///     - This must only be used on the main thread. It is not thread-safe
///     - The reference fizzles after subsequent calls to this function. 
///       The string must only be used for quick operations. 
///
/// If your use case does not adhere to these, you should 
/// allocate a new string instead.
const gensyn_string_t * gensyn_string_temporary_from_c_str(const char *);
#define GENSYN_STR_CAST(__s__) gensyn_string_temporary_from_c_str(__s__)


/// Sets the contents of the string A to the contents of string B
///
void gensyn_string_set(gensyn_string_t * A, const gensyn_string_t * B);

/// Resets the contents of the string.
///
void gensyn_string_clear(gensyn_string_t *);


/// Adds the given C printf-formatted string and accompanying arguments 
/// to the given string.
///
void gensyn_string_concat_printf(gensyn_string_t *, const char * format, ...);


/// Adds the given string B to the end of the given string A.
///
void gensyn_string_concat(gensyn_string_t *, const gensyn_string_t *);


/// Returns a read-only copy of a portion of the given string 
/// from and to denote character indices. The substring is valid until 
/// the next call to this function with the same input string.
const gensyn_string_t * gensyn_string_get_substr(
    const gensyn_string_t *, 
    uint32_t from, 
    uint32_t to
);



/// Gets a read-only pointer to a c-string representation of the 
/// string.
///
const char * gensyn_string_get_c_str(const gensyn_string_t *);

/// Gets the number of characters within the string.
///
uint32_t gensyn_string_get_length(const gensyn_string_t *);


/// Gets the byte length of the data representation 
/// of this string. Depending on the context, this could 
/// match the length of the string, or it could be wider.
///
uint32_t gensyn_string_get_byte_length(const gensyn_string_t *);

/// Gets the byte data pointer for this strings. Its length is equal to 
/// gensyn_string_get_byte_length()
///
void * gensyn_string_get_byte_data(const gensyn_string_t *);


//////// Tests
/// Returns whether substr is found within the given string 
///
int gensyn_string_test_contains(const gensyn_string_t *, const gensyn_string_t * substr);

/// Returns wither 2 strings are equivalent 
///
int gensyn_string_test_eq(const gensyn_string_t *, const gensyn_string_t * other);

/// Compares the 2 strings in a sort-ready fashion:
/// Returns < 0 if a alphabetically comes before b
/// Returns > 0 if a alphabetically comes after b
/// Returns = 0 if a and b are equivalent
int gensyn_string_gensyn_compare(const gensyn_string_t * a, const gensyn_string_t * b);





/////// Chain control
/// 
/// Chain functions can be used to 
/// work on strings in a token-like fashion.
/// Each token is referred to as a "link" in the chain.

/// Resets the chain state of the string. Using gensyn_string_chain_current()
/// will return the first token according to the delimiters given.
/// The first link is returned. If no such link exists, an empty 
/// link is returned.
///
const gensyn_string_t * gensyn_string_chain_start(gensyn_string_t * t, const gensyn_string_t * delimiters);

/// Returns the current link in the chain.
/// If the end has been reached, this is an empty string.
///
const gensyn_string_t * gensyn_string_chain_current(gensyn_string_t * t);

/// Returns whether the last link in the chain has been reached.
///
int gensyn_string_chain_is_end(const gensyn_string_t * t);

/// Goes to the next token in the chain and returns that token.
/// The new token is returned.
/// 
const gensyn_string_t * gensyn_string_chain_proceed(gensyn_string_t * t);

#endif


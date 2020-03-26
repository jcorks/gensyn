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


#ifndef H_GENSYNDC__ARRAY__INCLUDED
#define H_GENSYNDC__ARRAY__INCLUDED


#include <stdint.h>

/*

    Array
    -----

    Dynamically resizing container


*/
typedef struct gensyn_array_t gensyn_array_t;



/// Returns a new, empty array 
///
/// sizeofType refers to the size of the elements that the array will hold 
/// the most convenient way to do this is to use "sizeof()"
gensyn_array_t * gensyn_array_create(uint32_t sizeofType);

/// Destroys the container and buffer that it manages.
///
void gensyn_array_destroy(gensyn_array_t *);





/// Clones an entire array, returning a new array instance.
///
gensyn_array_t * gensyn_array_clone(const gensyn_array_t *);

/// Returns the size of the array
///
uint32_t gensyn_array_get_size(const gensyn_array_t *);

/// Returns the size of each element in bytes.
///
uint32_t gensyn_array_get_type_size(const gensyn_array_t *);

/// Adds an additional element to the array.
///
/// The value given should be an addressable value. For example a 
/// value "12" will not be able to be pushed, but if 
/// setting a variable ahead of time and adding 
/// "i = 12; gensyn_array_push(array, i);" would work.
#define gensyn_array_push(__A__, __VAL__) (gensyn_array_push_n(__A__, &__VAL__, 1))

/// Gets the value at the given index 
///
#define gensyn_array_at(__A__, __T__, __I__) (((__T__*)gensyn_array_get_data(__A__))[__I__])

/// Adds a contiguous set of elements to the array 
///
void gensyn_array_push_n(gensyn_array_t *, const void * element, uint32_t count);


/// Removes a specific member of the array
///
void gensyn_array_remove(gensyn_array_t *, uint32_t index);


/// Returns the index that this element should be inserted into 
/// given that the rest of the array is sorted.
///
uint32_t gensyn_array_lower_bound(
    const gensyn_array_t *, 

    /// The element in question
    ///    
    const void * element, 

    /// Returns whether the value that a points to is "less" than 
    /// the value that b points to.
    ///
    int(*less)(const void * a, const void * b)
);

/// Inserts the given value at the given position. 
///
#define gensyn_array_insert(__A__, __I__, __V__) (gensyn_array_insert_n(__A__, __I__, &(__V__), 1))



/// Inserts the given number of elements at the given index.
///
void gensyn_array_insert_n(gensyn_array_t *, uint32_t index, void * ele, uint32_t count);


/// Gets a pointer to the raw data of the array
/// This pointer is guaranteed to be a contiguous memory block of the 
/// current state of the array. It is editable.
///
void * gensyn_array_get_data(const gensyn_array_t *);


/// Clears the contents of the array.
/// After this operation, the size of the array
/// is 0.
///
void gensyn_array_clear(gensyn_array_t *);


/// Sets the size of the array
/// If the array were to be larger than it could account for,
/// this invokes an internal resize
///
void gensyn_array_set_size(gensyn_array_t *, uint32_t size);



#endif

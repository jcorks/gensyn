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

#include <gensyn/array.h>

#include <stdlib.h>
#include <string.h>

#ifdef GENSYNDC_DEBUG
#include <assert.h>
#endif


#define array_resize_amt 1.4
#define array_presize_amt 16

struct gensynArray_t {
    uint32_t allocSize;
    uint32_t size;
    uint32_t sizeofType;
    uint8_t * data;
};


gensynArray_t * gensyn_array_create(uint32_t typesize) {
    gensynArray_t * a = malloc(sizeof(gensynArray_t));
    a->sizeofType = typesize;
    a->size = 0;
    a->allocSize = array_presize_amt;
    a->data = malloc(typesize*array_presize_amt);
    return a;
}

void gensyn_array_destroy(gensynArray_t * t) {
    #ifdef GENSYNDC_DEBUG
        assert(t && "gensynArray_t pointer cannot be NULL.");
    #endif
    free(t->data);
    free(t);
}



gensynArray_t * gensyn_array_clone(const gensynArray_t * src) {
    #ifdef GENSYNDC_DEBUG
        assert(src && "gensynArray_t pointer cannot be NULL.");
    #endif
    gensynArray_t * a = malloc(sizeof(gensynArray_t));

    // do not clone pre-alloc size
    a->allocSize = src->size;
    a->size = src->size;
    a->sizeofType = src->sizeofType;
    a->data = malloc(src->size*a->sizeofType);
    memcpy(a->data, src->data, src->size*a->sizeofType);
    return a;
}

uint32_t gensyn_array_lower_bound(const gensynArray_t * t, const void * ptrToEle, int(*comp)(const void * a, const void * b)) {
    int64_t lo = 0;
    int64_t hi = t->size;
    int64_t mid;
    while(lo < hi) {
        mid = lo + ((hi-lo) >> 1);
        void * val = t->data + t->sizeofType*mid;
        if (comp(ptrToEle, val)) {
            hi = mid;
        } else {
            lo = mid+1;
        }
    }

    return lo;
}

void gensyn_array_insert_n(gensynArray_t * t, uint32_t index, void * ele, uint32_t count) {
    gensyn_array_set_size(t, t->size+count);

    memmove(
        t->data+(t->sizeofType*(index+count)),
        t->data+(t->sizeofType*index),
        t->sizeofType*(t->size-index)
    );

    memcpy(
        t->data+(t->sizeofType*index),
        ele,
        count*(t->sizeofType)
    );
}


uint32_t gensyn_array_get_size(const gensynArray_t * t) {
    #ifdef GENSYNDC_DEBUG
        assert(t && "gensynArray_t pointer cannot be NULL.");
    #endif
    return t->size;
}

uint32_t gensyn_array_get_type_size(const gensynArray_t * t) {
    #ifdef GENSYNDC_DEBUG
        assert(t && "gensynArray_t pointer cannot be NULL.");
    #endif
    return t->sizeofType;
}

void gensyn_array_push_n(gensynArray_t * t, const void * elements, uint32_t count) {
    #ifdef GENSYNDC_DEBUG
        assert(t && "gensynArray_t pointer cannot be NULL.");
    #endif
    while(t->size + count > t->allocSize) {
        t->allocSize += t->allocSize*array_presize_amt;
        t->data = realloc(t->data, t->allocSize*t->sizeofType);
    }
    memcpy(
        (t->data)+(t->size*t->sizeofType), 
        elements, 
        count*t->sizeofType
    );
    t->size++;
}


void gensyn_array_remove(gensynArray_t * t, uint32_t index) {
    #ifdef GENSYNDC_DEBUG
        assert(t && "gensynArray_t pointer cannot be NULL.");
        assert(index < t->size);
    #endif

    uint32_t indexByte = index*t->sizeofType;
    memmove(
        t->data+(indexByte),
        t->data+(indexByte + t->sizeofType),
        ((t->size-1)*t->sizeofType) - indexByte
    );
    t->size--;
}



void * gensyn_array_get_data(const gensynArray_t * t) {
    #ifdef GENSYNDC_DEBUG
        assert(t && "gensynArray_t pointer cannot be NULL.");
    #endif
    return t->data;
}


void gensyn_array_clear(gensynArray_t * t) {
    #ifdef GENSYNDC_DEBUG
        assert(t && "gensynArray_t pointer cannot be NULL.");
    #endif
    t->size = 0;
}

void gensyn_array_set_size(gensynArray_t * t, uint32_t size) {
    #ifdef GENSYNDC_DEBUG
        assert(t && "gensynArray_t pointer cannot be NULL.");
    #endif
    while(size > t->allocSize) {
        t->allocSize += t->allocSize*array_presize_amt;
        t->data = realloc(t->data, t->allocSize*t->sizeofType);
    }
    t->size = size;
}

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

#include <gensyn/string.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef GENSYNDC_DEBUG
#include <assert.h>
#endif



#define prealloc_size 32


struct gensyn_string_t {
    char * cstr;
    uint32_t len;
    uint32_t alloc;

    gensyn_string_t * delimiters;
    gensyn_string_t * chain;
    uint32_t iter;

    gensyn_string_t * lastSubstr;
};

static void gensyn_string_concat_cstr(gensyn_string_t * s, const char * cstr, uint32_t len) {
    while (s->len + len + 1 >= s->alloc) {
        s->alloc*=1.4;
        s->cstr = realloc(s->cstr, s->alloc);
    }

    memcpy(s->cstr+s->len, cstr, len+1);
    s->len+=len;
}

static void gensyn_string_set_cstr(gensyn_string_t * s, const char * cstr, uint32_t len) {
    s->len = 0;
    gensyn_string_concat_cstr(s, cstr, len);
}




gensyn_string_t * gensyn_string_create() {
    gensyn_string_t * out = calloc(1, sizeof(gensyn_string_t));
    out->alloc = prealloc_size;
    out->cstr = malloc(prealloc_size);
    out->cstr[0] = 0;
    return out;
}

gensyn_string_t * gensyn_string_create_from_c_str(const char * format, ...) {
    va_list args;
    va_start(args, format);
    int lenReal = vsnprintf(NULL, 0, format, args);
    va_end(args);


    char * newBuffer = malloc(lenReal+2);
    va_start(args, format);    
    vsnprintf(newBuffer, lenReal+1, format, args);
    va_end(args);
    

    gensyn_string_t * out = gensyn_string_create();
    gensyn_string_set_cstr(out, newBuffer, lenReal);
    free(newBuffer);
    return out;
}

gensyn_string_t * gensyn_string_clone(const gensyn_string_t * src) {
    gensyn_string_t * out = gensyn_string_create();
    gensyn_string_set(out, src);
    return out;
}

void gensyn_string_destroy(gensyn_string_t * s) {
    free(s->cstr);
    if (s->delimiters) gensyn_string_destroy(s->delimiters);
    if (s->chain) gensyn_string_destroy(s->chain);
    if (s->lastSubstr) gensyn_string_destroy(s->lastSubstr);
    free(s);
}

void gensyn_string_clear(gensyn_string_t * s) {
    s->len = 0;
}

void gensyn_string_set(gensyn_string_t * s, const gensyn_string_t * src) {
    free(s->cstr);
    s->len = src->len;
    s->alloc = src->alloc;
    s->cstr = malloc(s->alloc);
    memcpy(s->cstr, src->cstr, src->len+1);

    if (s->delimiters) gensyn_string_destroy(s->delimiters);
    if (s->chain) gensyn_string_destroy(s->chain);

}



void gensyn_string_concat_printf(gensyn_string_t * s, const char * format, ...) {
    va_list args;
    va_start(args, format);
    int lenReal = vsnprintf(NULL, 0, format, args);
    va_end(args);


    char * newBuffer = malloc(lenReal+2);
    va_start(args, format);    
    vsnprintf(newBuffer, lenReal+1, format, args);
    va_end(args);
    


    gensyn_string_concat_cstr(s, newBuffer, lenReal);
    free(newBuffer);
}

void gensyn_string_concat(gensyn_string_t * s, const gensyn_string_t * src) {
    gensyn_string_concat_cstr(s, src->cstr, src->len);
}



const gensyn_string_t * gensyn_string_get_substr(
    const gensyn_string_t * s,
    uint32_t from,
    uint32_t to
) {
    #ifdef GENSYNDC_DEBUG
        assert(from < s->len);
        assert(to < s->len);
    #endif

    if (to < from) {
        uint32_t temp = to;
        to = from;
        from = temp;
    }

    if (!s->lastSubstr) {
        ((gensyn_string_t *)s)->lastSubstr = gensyn_string_create();
    }

    char realAtTo = s->cstr[to];
    s->cstr[to] = 0;
    gensyn_string_set_cstr(
        s->lastSubstr, 
        s->cstr+from, 
        to - from
    );
    s->cstr[to] = realAtTo;    

    return s->lastSubstr;    
}



const char * gensyn_string_get_c_str(const gensyn_string_t * t) {
    return t->cstr;
}

uint32_t gensyn_string_get_length(const gensyn_string_t * t) {
    return t->len;
}

uint32_t gensyn_string_get_byte_length(const gensyn_string_t * t) {
    // for now same as string length. will change when unicode is supported.
    return t->len;
}

void * gensyn_string_get_byte_data(const gensyn_string_t * t) {
    return t->cstr;
}

int gensyn_string_test_contains(const gensyn_string_t * a, const gensyn_string_t * b) {
    return strstr(a->cstr, b->cstr) != NULL;
}

int gensyn_string_test_eq(const gensyn_string_t * a, const gensyn_string_t * b) {
    return strcmp(a->cstr, b->cstr) == 0;
}

int gensyn_string_gensyn_compare(const gensyn_string_t * a, const gensyn_string_t * b) {
    return strcmp(a->cstr, b->cstr);
}





const gensyn_string_t * gensyn_string_chain_start(gensyn_string_t * t, const gensyn_string_t * delimiters) {
    t->iter = 0;
    if (!t->chain) {
        t->chain = gensyn_string_create();
        t->delimiters = gensyn_string_create();
    }
    gensyn_string_set(t->delimiters, delimiters);
    return gensyn_string_chain_proceed(t);
}

const gensyn_string_t * gensyn_string_chain_current(gensyn_string_t * t) {
    if (!t->chain) {
        t->chain = gensyn_string_create();
        t->delimiters = gensyn_string_create();
    }
    return t->chain;
}

int gensyn_string_chain_is_end(const gensyn_string_t * t) {
    return t->iter > t->len;
}

const gensyn_string_t * gensyn_string_chain_proceed(gensyn_string_t * t) {

    char * del = t->delimiters->cstr;
    char * iter;    

    #define chunk_size 32
    char chunk[chunk_size];
    uint32_t chunkLen = 0;


    char c;

    // skip over leading delimiters
    for(; t->iter < t->len; t->iter++) {
        c = t->cstr[t->iter];

        // delimiter marks the end.
        for(iter = del; *iter; ++iter) {
            if (*iter == c) {
                break;                
            }
        }
        if (!*iter) break;
    }

    // reset for next link
    gensyn_string_set_cstr(t->chain, "", 0);

    // check if at end
    if (t->iter >= t->len) {
        t->iter = t->len+1;
        return t->chain;
    }

    for(; t->iter < t->len; t->iter++) {
        c = t->cstr[t->iter];

        // delimiter marks the end.
        for(iter = del; *iter; ++iter) {
            if (*iter == c && chunkLen) {
                chunk[chunkLen] = 0;
                gensyn_string_concat_cstr(t->chain, chunk, chunkLen);                
                return t->chain;
            }
        }         

        if (chunkLen == chunk_size-1) {
            chunk[chunkLen] = 0;
            gensyn_string_concat_cstr(t->chain, chunk, chunkLen);                
            chunkLen = 0;
        }
        chunk[chunkLen++] = c;
   }

    // reached the end of the string 
    t->iter = t->len;
    chunk[chunkLen] = 0;
    gensyn_string_concat_cstr(t->chain, chunk, chunkLen);         
    return t->chain;       
}







#define gensyn_string_temp_max_calls 128
static gensyn_string_t * tempVals[gensyn_string_temp_max_calls];
static int tempIter = 0;
static int tempInit = 0;

const gensyn_string_t * gensyn_string_temporary_from_c_str(const char * s) {
    if (!tempInit) {
        uint32_t i;
        for(i = 0; i < gensyn_string_temp_max_calls; ++i)
            tempVals[i] = gensyn_string_create();
        tempInit = 1;
    }    

    if (tempIter >= gensyn_string_temp_max_calls) tempIter = gensyn_string_temp_max_calls;
    gensyn_string_t * t = tempVals[tempIter++];
    gensyn_string_set_cstr(t, s, strlen(s));
    return t;
}









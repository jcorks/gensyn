#include "srgs.h"


#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>

#define SRGS_DEADADDRESSTAG ((void*)0xB00)

// manages IDs in a similar manner to pointers
typedef struct srgs_id_table_t srgs_id_table;
typedef struct {
    uint32_t size;
    uint32_t sizeDead;
    uint32_t sizeAlloc;
    uint32_t sizeDeadAlloc;
    uint32_t sizeofTypeInt;


    int * alloc; // for unsafe, quick fetches.


    uint32_t * dead;
    srgs_t * ctx;
} srgs_id_table_t;




// creates a new table to manage IDs
static srgs_id_table_t * id_table_create(srgs_t *, uint32_t sizeOfType);

// returns 1 if the ID refers to a valid entry in the table.
// An invalid entry is one that:
// - Was removed and was never reallocated.
// - Was never given
static int id_table_verify(const srgs_id_table_t *, uint32_t id);

// frees a table and all its resources
static void id_table_destroy(srgs_id_table_t *);

// returns a new ID
static uint32_t id_table_new_id(srgs_id_table_t *);

// Returns a new array of all IDs that are alive.
// The returned buffer needs to be freed.
static uint32_t * id_table_get_alive_ids(srgs_id_table_t *, uint32_t * count);

// __T__ -> table 
// __TYPE__ -> type 
// __I__ -> index
#define id_table_fetch(__T__, __TYPE__, __I__) ((__TYPE__ *)(__T__->alloc+__I__))


// recycles an ID. will be possible to use it again once its 
// reissued.
static void id_table_recycle_id(srgs_id_table_t *, uint32_t);













// Copies an array of aligned ints.
//
static void srgs_memcpy_int(void * dest, const void * src, int numElements) {
    memcpy(dest, src, numElements*sizeof(int));
}


// Copies an array of aligned floats.
//
static void srgs_memcpy_float(void * dest, const void * src, int numElements) {
    memcpy(dest, src, numElements*sizeof(float));

}



typedef struct {
    uint32_t w;
    uint32_t h;
    uint8_t * data;
} srgs_texture_t;

#define SRGS__FLOATS_PER_VERTEX (3+2+4)

typedef struct {
    uint32_t texture;
    uint32_t vertexCount;
    uint32_t matrixID;
    uint32_t indexCount;
    srgs__object_render_mode renderMode;
    srgs__object_depth_mode depthMode;
    
    // xyz
    // uv
    // abcd
    float  * verticesInterleaved;
    uint32_t * indices;
} srgs_object_t;





typedef struct {
    uint32_t size;
    uint32_t viewID;
    uint32_t projID;
    uint32_t * objects;
   
} srgs_renderlist_t;




struct srgs_t {
    void * (*alloc)(size_t);
    void   (*deloc)(void *);
    void * (*reloc)(void *, size_t);
    
    srgs_id_table_t * textures;
    srgs_id_table_t * objects;
    srgs_id_table_t * renderLists;
    srgs_id_table_t * matrices;

    uint32_t framebufferID;
    uint32_t depthbufferID;

};













srgs_t * srgs_create(
    uint32_t startW, uint32_t startH,

    // can be NULL. In that case, malloc() is used.
    void * (*allocatorFunction)  (size_t),

    // can be NULL. In that case, free() is used.
    void   (*deallocatorFunction)(void *),

    // can be NULL. In that case, realloc() is used.
    void * (*reallocatorFunction)(void *, size_t)


) {
    uint32_t nullID;
    uint8_t nullID_data[16] = {
        0x14, 0x14, 0x14, 0xff,
        0xFF, 0x00, 0xFF, 0xff,
        0xFF, 0x00, 0xFF, 0xff,
        0x14, 0x14, 0x14, 0xff
    };




    if (!allocatorFunction)   allocatorFunction   = malloc;
    if (!deallocatorFunction) deallocatorFunction = free;
    if (!reallocatorFunction) reallocatorFunction = realloc;

    srgs_t * out = allocatorFunction(sizeof(srgs_t));
    out->alloc = allocatorFunction;
    out->deloc = deallocatorFunction;
    out->reloc = reallocatorFunction;

    out->textures    = id_table_create(out, sizeof(srgs_texture_t));
    out->objects     = id_table_create(out, sizeof(srgs_object_t));
    out->renderLists = id_table_create(out, sizeof(srgs_renderlist_t));
    out->matrices    = id_table_create(out, sizeof(srgs_matrix_t));



    // textureID 0 is the NULL texture, as an homage to the source engine
    nullID = srgs_texture_create(out, 2, 2);
    srgs_texture_update(out, nullID,
        nullID_data,

        0, 0,
        0, 0,
        2, 2
    );


    // next should always be the framebuffer
    out->framebufferID = srgs_texture_create(out, startW, startW);

    // next should be the depthbuffer
    out->depthbufferID = srgs_texture_create(out, startW, startW);


    // matrix id 0 is always identity.
    srgs_matrix_create(out);

    return out;
}


uint32_t srgs_get_framebuffer_texture(const srgs_t * t) {
    return t->framebufferID;
}

uint32_t srgs_get_depthbuffer_texture(const srgs_t * t) {
    return t->depthbufferID;
}

void srgs_destroy(srgs_t * s) {
    uint32_t i; 
    uint32_t count;
    uint32_t * alive;


    alive = id_table_get_alive_ids(s->textures, &count);    
    for(i = 0; i < count; ++i) {
        srgs_texture_destroy(s, alive[i]);
    }
    s->deloc(alive);

    alive = id_table_get_alive_ids(s->objects, &count);
    for(i = 0; i < count; ++i) {
        srgs_object_destroy(s, alive[i]);
    }

    s->deloc(alive);
    alive = id_table_get_alive_ids(s->renderLists, &count);
    for(i = 0; i < count; ++i) {
        srgs_renderlist_destroy(s, alive[i]);
    }
    s->deloc(alive);



    id_table_destroy(s->textures);
    id_table_destroy(s->objects);
    id_table_destroy(s->renderLists);
    id_table_destroy(s->matrices);


    s->deloc(s);
}




void srgs_clear_depth(srgs_t * s) {
    srgs_texture_blank(s, s->depthbufferID, 0xff);
}

void srgs_clear_color(srgs_t * s) {
    srgs_texture_blank(s, s->framebufferID, 0x00);
}






uint32_t srgs_texture_create(srgs_t * t, uint32_t w, uint32_t h) {
    uint32_t id = id_table_new_id(t->textures);
    srgs_texture_t * out = id_table_fetch(t->textures, srgs_texture_t, id);

    out->w = w;
    out->h = h;
    out->data = t->alloc(w*h*sizeof(uint32_t));
    return id;
}


int srgs_texture_verify(srgs_t * t, uint32_t id) {
    return id_table_verify(t->textures, id);
}

void srgs_texture_destroy(srgs_t * t, uint32_t id) {
    srgs_texture_t * tex = id_table_fetch(t->textures, srgs_texture_t, id);
    t->deloc(tex->data);
    tex->data = SRGS_DEADADDRESSTAG;
    id_table_recycle_id(t->textures, id);
}

void srgs_texture_blank(srgs_t * t, uint32_t id, uint8_t white) {
    srgs_texture_t * tex = id_table_fetch(t->textures, srgs_texture_t, id);
    uint32_t i;
    uint32_t count = tex->w * tex->h;
    memset(tex->data, white, count*4);
}

void srgs_texture_update(
    srgs_t * t, 
    uint32_t id,

    const uint8_t * src, 
    uint32_t xDest, uint32_t yDest, 
    uint32_t xSrc,  uint32_t ySrc,
    uint32_t wSrc,  uint32_t hSrc
) {
    srgs_texture_t * tex = id_table_fetch(t->textures, srgs_texture_t, id);
    
          uint32_t * to;
    const uint32_t * from;

    uint32_t y;


    for(y = 0; y < hSrc; ++y) {
        to    = ((      uint32_t*)tex->data)+(xDest + tex->w*(yDest+y));
        from  = ((const uint32_t*)src      )+(xSrc  + wSrc  *(ySrc +y));
        srgs_memcpy_int(to, from, wSrc);
        

    }
}



void srgs_texture_resize(srgs_t * t, uint32_t id, uint32_t w, uint32_t h) {
    srgs_texture_t * tex = id_table_fetch(t->textures, srgs_texture_t, id);

    tex->data = t->reloc(tex->data, w*h*4);
    tex->w = w;
    tex->h = h;
}



const uint8_t * srgs_texture_get_data(const srgs_t * t, uint32_t id) {
    return id_table_fetch(t->textures, srgs_texture_t, id)->data;
}


uint32_t srgs_texture_get_width(const srgs_t * t, uint32_t id) {
    return id_table_fetch(t->textures, srgs_texture_t, id)->w;
}

uint32_t srgs_texture_get_height(const srgs_t * t, uint32_t id) {
    return id_table_fetch(t->textures, srgs_texture_t, id)->h;
}









static float identityMatrix[16] = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

uint32_t srgs_matrix_create(srgs_t * t) {
    uint32_t id = id_table_new_id(t->matrices);
    srgs_memcpy_float(id_table_fetch(t->matrices, srgs_matrix_t, id), identityMatrix, 16);    
    return id;
}


int srgs_matrix_verify(srgs_t * t, uint32_t id) {
    return id_table_verify(t->matrices, id);
}

void srgs_matrix_destroy(srgs_t * t, uint32_t id) {
    id_table_recycle_id(t->matrices, id);
}




void srgs_matrix_set(srgs_t * t, uint32_t id, srgs_matrix_t * f16) {
    srgs_matrix_t * m = id_table_fetch(t->matrices, srgs_matrix_t, id);
    srgs_memcpy_float(m, f16, 16);    
}


const srgs_matrix_t * srgs_matrix_get(const srgs_t * t, uint32_t id) {
    return id_table_fetch(t->matrices, srgs_matrix_t, id);
}















uint32_t srgs_object_create(srgs_t * t) {
    uint32_t id = id_table_new_id(t->objects);
    srgs_object_t * out = id_table_fetch(t->objects, srgs_object_t, id);
    
    out->texture = 0;
    out->vertexCount = 0;
    out->renderMode = srgs__object_render_mode__color;
    out->depthMode = srgs__object_depth_mode__less;
    out->verticesInterleaved = NULL;
    out->matrixID = 0;
    out->indices = NULL;
    out->indexCount = 0;
    return id;
}



int srgs_object_verify(srgs_t * t, uint32_t id) {
    return id_table_verify(t->objects, id);
}

uint32_t srgs_object_clone(srgs_t * t, uint32_t idIn) {
    uint32_t id = id_table_new_id(t->objects);
    srgs_object_t * out = id_table_fetch(t->objects, srgs_object_t, id);
    srgs_object_t * old = id_table_fetch(t->objects, srgs_object_t, idIn);

    out->texture = old->texture;
    out->matrixID = old->matrixID;
    out->vertexCount = old->vertexCount;
    out->indexCount = old->indexCount;
    out->renderMode = old->renderMode;
    out->depthMode = old->depthMode;
    out->verticesInterleaved = t->alloc(out->vertexCount*SRGS__FLOATS_PER_VERTEX);
    srgs_memcpy_float(
        out->verticesInterleaved,
        old->verticesInterleaved,
        SRGS__FLOATS_PER_VERTEX * out->vertexCount
    );
    out->indices = t->alloc(out->indexCount*sizeof(uint32_t));
    srgs_memcpy_int(
        out->indices,
        old->indices,
        sizeof(uint32_t) * out->indexCount
    );

    return id;
}


void srgs_object_destroy(srgs_t * t, uint32_t id) {
    srgs_object_t * out = id_table_fetch(t->objects, srgs_object_t, id);
    t->deloc(out->verticesInterleaved);
    t->deloc(out->indices);
    out->verticesInterleaved = SRGS_DEADADDRESSTAG;
    out->indices = SRGS_DEADADDRESSTAG;
    id_table_recycle_id(t->objects, id);    
}

void srgs_object_set_vertex_count(srgs_t * t, uint32_t id, uint32_t vcount) {
    srgs_object_t * o = id_table_fetch(t->objects, srgs_object_t, id);   
    o->verticesInterleaved = t->reloc(o->verticesInterleaved, vcount*SRGS__FLOATS_PER_VERTEX*sizeof(float));
    o->vertexCount = vcount;
}


void srgs_object_define_vertices(
    srgs_t *  t, 
    uint32_t  id, 
    srgs__object_vertex_channel  c,     
    float *   src
) {
    srgs_object_t * o = id_table_fetch(t->objects, srgs_object_t, id);   

    srgs_object_update_vertices(
        t,
        id,
        c,
        0,
        o->vertexCount-1,
        src
    );
}



void srgs_object_update_vertices(
    srgs_t * t, 
    uint32_t id, 
    srgs__object_vertex_channel c, 
    uint32_t fromVertexIndex, 
    uint32_t toVertexIndex, 
    float * src
) {
    srgs_object_t * o = id_table_fetch(t->objects, srgs_object_t, id);   
    uint32_t i;
    float * iter = o->verticesInterleaved;
    switch(c) {
      case srgs__object_vertex_channel__position:
        for(i = fromVertexIndex; i <= toVertexIndex; ++i) {
            iter[0] = src[0];
            iter[1] = src[1];
            iter[2] = src[2];
            iter += SRGS__FLOATS_PER_VERTEX;
            src  += 3;
        }
        break;

      case srgs__object_vertex_channel__uvs:
        iter += 3;
        for(i = fromVertexIndex; i <= toVertexIndex; ++i) {
            iter[0] = src[0];
            iter[1] = src[1];
            iter += SRGS__FLOATS_PER_VERTEX;
            src  += 2;
        }
        break;

      case srgs__object_vertex_channel__color:
        iter += 5;
        for(i = fromVertexIndex; i <= toVertexIndex; ++i) {
            iter[0] = src[0];
            iter[1] = src[1];
            iter[2] = src[2];
            iter[3] = src[3];
            iter += SRGS__FLOATS_PER_VERTEX;
            src  += 4;
        }
        break;
    }


}


const float * srgs_object_get_vertices(
    const srgs_t * s, 
    int32_t id
) {
    srgs_object_t * o = id_table_fetch(s->objects, srgs_object_t, id);   
    return o->verticesInterleaved;
}


void srgs_object_define_indices(
    srgs_t * s, 
    uint32_t id, 
    uint32_t count, 
    uint32_t * indices
) {
    srgs_object_t * o = id_table_fetch(s->objects, srgs_object_t, id);   
    s->deloc(o->indices);
    o->indices = s->alloc(count*sizeof(uint32_t));
    o->indexCount = count;
    srgs_memcpy_int(
        o->indices,
        indices,
        count
    );
}




void srgs_object_set_render_mode(srgs_t * t, uint32_t id, srgs__object_render_mode mode) {
    srgs_object_t * o = id_table_fetch(t->objects, srgs_object_t, id);   
    o->renderMode = mode;
}

void srgs_object_set_depth_mode(srgs_t * t, uint32_t id, srgs__object_depth_mode mode) {
    srgs_object_t * o = id_table_fetch(t->objects, srgs_object_t, id);   
    o->depthMode = mode;
}

void srgs_object_set_texture(srgs_t * t, uint32_t id, uint32_t tex) {
    srgs_object_t * o = id_table_fetch(t->objects, srgs_object_t, id);
    o->texture = tex;
}

void srgs_object_set_transform(srgs_t * t, uint32_t id, uint32_t m) {
    srgs_object_t * o = id_table_fetch(t->objects, srgs_object_t, id);
    o->matrixID = m;
}


void srgs_object_get_parameters(
    const srgs_t * t,
    uint32_t id,
    
    srgs__object_render_mode * render,
    srgs__object_depth_mode * depth,
    uint32_t * matrixID,
    uint32_t * textureID,
    uint32_t * vertexCount 
) {
    srgs_object_t * o = id_table_fetch(t->objects, srgs_object_t, id);   
    
    if (render)     *render      = o->renderMode;
    if (depth)      *depth       = o->depthMode;
    if (matrixID)   *matrixID    = o->matrixID;        
    if (textureID)  *textureID   = o->texture;        
    if (vertexCount)*vertexCount = o->vertexCount;
    
}   














/*
    uint32_t size;
    uint32_t * objects;
*/
uint32_t srgs_renderlist_create(srgs_t * t) {
    uint32_t id = id_table_new_id(t->renderLists);
    srgs_renderlist_t * out = id_table_fetch(t->renderLists, srgs_renderlist_t, id);
    out->size = 0;
    out->projID = 0;
    out->viewID = 0;
    out->objects = NULL;  
    return id;
}


int srgs_renderlist_verify(srgs_t * t, uint32_t id) {
    return id_table_verify(t->renderLists, id);
}

void srgs_renderlist_set_projection_transform(srgs_t * t, uint32_t id, uint32_t matrix) {
    srgs_renderlist_t * l = id_table_fetch(t->renderLists, srgs_renderlist_t, id);
    l->projID = matrix;    
}

void srgs_renderlist_set_view_transform(srgs_t * t, uint32_t id, uint32_t matrix) {
    srgs_renderlist_t * l = id_table_fetch(t->renderLists, srgs_renderlist_t, id);
    l->viewID = matrix;    
}


void srgs_renderlist_destroy(srgs_t * t, uint32_t id) {
    srgs_renderlist_t * l = id_table_fetch(t->renderLists, srgs_renderlist_t, id);
    t->deloc(l->objects);
    l->objects = SRGS_DEADADDRESSTAG;
    id_table_recycle_id(t->renderLists, id);
}


void srgs_renderlist_set_objects(srgs_t * t, uint32_t id, uint32_t count, uint32_t * renderListIDs) {
    srgs_renderlist_t * l = id_table_fetch(t->renderLists, srgs_renderlist_t, id);
    t->deloc(l->objects);
    l->objects = t->alloc(count*sizeof(uint32_t));
    l->size = count;
    srgs_memcpy_int(l->objects, renderListIDs, count);
}









////////////////////
/// id_table;


#define SGS_ID_TABLE__DEFAULT_ALLOC 32
#define SGS_ID_TABLE__ALLOC_CHUNK   32



srgs_id_table_t * id_table_create(srgs_t * s, uint32_t sizeofType) {
    assert((sizeofType/sizeof(int))*sizeof(int) == sizeofType); // MUST be int aligned


    srgs_id_table_t * out = s->alloc(sizeof(srgs_id_table_t));
    out->ctx = s;
    out->sizeAlloc = SGS_ID_TABLE__DEFAULT_ALLOC;
    out->size = 0;
    out->sizeDead = 0;
    out->sizeDeadAlloc = 0;
    out->sizeofTypeInt = sizeofType / sizeof(uint32_t);
    out->alloc = s->alloc(sizeof(uint32_t)*out->sizeAlloc*out->sizeofTypeInt);
    out->dead = NULL;
}

int id_table_verify(const srgs_id_table_t * t, uint32_t id) {
    uint32_t i;
    if (id >= t->size*t->sizeofTypeInt) return 0;

    for(i = 0; i < t->sizeDead; ++i) {
        if (t->dead[i] == id) return 0;
    }
    return 1;
}

void id_table_destroy(srgs_id_table_t * t) {
    t->ctx->deloc(t->alloc);
    t->ctx->deloc(t->dead);
    t->ctx->deloc(t);
}

uint32_t id_table_new_id(srgs_id_table_t * t) {
    if (t->sizeDead) {
        t->sizeDead--;
        return t->dead[t->sizeDead];
    }

    // realloc
    if (t->size >= t->sizeAlloc) {
        t->sizeAlloc += SGS_ID_TABLE__ALLOC_CHUNK;
        t->alloc = t->ctx->reloc(t->alloc, t->sizeAlloc*t->sizeofTypeInt*sizeof(int));
    }
    
    return (t->size++)*t->sizeofTypeInt;
}

void id_table_recycle_id(srgs_id_table_t * t, uint32_t id) {
    if (t->sizeDead >= t->sizeDeadAlloc) {
        t->sizeDeadAlloc += SGS_ID_TABLE__ALLOC_CHUNK;
        t->dead = t->ctx->reloc(t->dead, t->sizeDeadAlloc*(sizeof(uint32_t)));
    }

    t->dead[t->sizeDead++] = id;
}

uint32_t * id_table_get_alive_ids(srgs_id_table_t * t, uint32_t * count) {
    uint32_t * outpre  = t->ctx->alloc(t->size*sizeof(uint32_t));
    uint32_t * outreal = t->ctx->alloc(t->size*sizeof(uint32_t));
    uint32_t i;

    for(i = 0; i < t->size; ++i) {
        outpre[i] = i*t->sizeofTypeInt;
    }
    for(i = 0; i < t->sizeDead; ++i) {
        outpre[t->dead[i]/t->sizeofTypeInt] = 0xfffffffe;
    }

    *count = 0;
    for(i = 0; i < t->size; ++i) {
        if (outpre[i] != 0xfffffffe)
            outreal[(*count)++] = outpre[i];
    }

    t->ctx->deloc(outpre);
    return outreal;

}































srgs_vector3_t VEC3(float x, float y, float z) {
    srgs_vector3_t out;
    out.x = x;
    out.y = y;
    out.z = z;
    return out;
}

srgs_vector4_t VEC4(float x, float y, float z, float w) {
    srgs_vector4_t out;
    out.x = x;
    out.y = y;
    out.z = z;
    out.w = w;
    return out;
}


void srgs_utility_vec_set(srgs_vector4_t * vec, float x, float y, float z, float w) {
	vec->x = x;
	vec->y = y;
	vec->z = z;
	vec->w = w;
}

void srgs_utility_vec_normalize(srgs_vector3_t * v) {
    float mag = sqrt(v->x*v->x + v->y*v->y+v->z*v->z);
    v->x /= mag;
    v->y /= mag;
    v->z /= mag;
}

float srgs_utility_distance(const srgs_vector3_t * a, const srgs_vector3_t * b) {
    srgs_vector3_t ab = {
        a->x - b->x,
        a->y - b->y,
        a->z - b->z        
    };
    return srgs_utility_vec_magnitude(&ab);
}


int srgs_utility_intersection_test(
    const srgs_vector3_t * lineA,
    const srgs_vector3_t * lineB,
    
    const srgs_vector3_t * p0,
    const srgs_vector3_t * p1,
    const srgs_vector3_t * p2,
    
    srgs_vector3_t * result
) {
    
    srgs_vector3_t out;
    srgs_vector3_t p_0 = *p0, 
              p_1 = *p1,
              p_2 = *p2; 

    srgs_vector3_t l_a = *lineA, 
              l_b = *lineB;

    srgs_vector3_t d   = {l_b.x - l_a.x,
                     l_b.y - l_a.y,
                     l_b.z - l_a.z};

    
    
    
    srgs_vector3_t n;
    {
        srgs_vector3_t p_21 = {p_2.x - p_1.x,
                          p_2.y - p_1.y,
                          p_2.z - p_1.z};

        srgs_vector3_t p_01 = {p_0.x - p_1.x,
                          p_0.y - p_1.y,
                          p_0.z - p_1.z};
        srgs_utility_vec_cross(&n, &p_01, &p_21);
        srgs_utility_vec_normalize(&n);
    }
    
    
    double planeD  = -(p_1.x*n.x + p_1.y*n.y + p_1.z*n.z);
    double t = - (n.x*l_a.x + n.y*l_a.y + n.z*l_a.z + planeD)/
                 (n.x*d.x   + n.y*d.y   + n.z*d.z);
    out.x = l_a.x + (d.x*t);
    out.y = l_a.y + (d.y*t);
    out.z = l_a.z + (d.z*t);



    // inefficent but will work: reduce to a 2d projection and perform the 
    // polygon in/out test.
    
    srgs_vector3_t eye = srgs_utility_surface_normal(&p_0, &p_1, &p_2);
    srgs_vector3_t center = {
        (p_0.x + p_1.x + p_2.x) / 3.f,
        (p_0.y + p_1.y + p_2.y) / 3.f,
        (p_0.z + p_1.z + p_2.z) / 3.f
    };
    srgs_vector3_t up = {
        p_0.x - center.x,
        p_0.y - center.y,
        p_0.z - center.z
    };
    srgs_utility_vec_normalize(&eye);
    srgs_utility_vec_normalize(&up);

    eye.x += center.x;
    eye.y += center.y;
    eye.z += center.z;

    // get the rotation + translation to make sure that the triangle is not coplanar
    srgs_matrix_t transform = *srgs_utility_matrix_get_identity();
    srgs_utility_matrix_look_at(&transform, &eye, &center, &up);
    
    srgs_vector3_t p_2d[3] = {
        srgs_utility_matrix_transform(&transform, &p_0),
        srgs_utility_matrix_transform(&transform, &p_1),
        srgs_utility_matrix_transform(&transform, &p_2)
    };
    srgs_vector3_t out_2d = srgs_utility_matrix_transform(&transform, &out);
    
    // now that we have the intersection, we need to see 
    // if the point is within the surface definition
    


    int k, i;
    int in = 0;
    srgs_vector3_t * this = NULL;
    srgs_vector3_t * next = NULL;

    float x = out_2d.x;
    float y = out_2d.y;

    for(i = 0, k = 3 - 1; i < 3; k = i++) { // flow control for wrapping behavior. i is current poly, n is next
        this = p_2d+i;
        next = p_2d+k;

        if (
                ((this->y > y) != 
                 (next->y > y))
            
            &&
            
                (x < (next->x - this->x) * (y - this->y) / (next->y - this->y) + this->x)
            
            ) {
            in = !(in);    
        
        }
    }
    
    
    if (in) {
        *result = out;
        return 1;
    } else {
        return 0;
    }
}


// adapted from blender src
int srgs_utility_intersection_test_plane(
    const srgs_vector3_t * linePoint,
    const srgs_vector3_t * linePoint2,
    
    const srgs_vector3_t * planePoint,
    const srgs_vector3_t * planeNormal,
    
    srgs_vector3_t * intersectResult
 
) {
    srgs_vector3_t u = {
        linePoint2->x - linePoint->x,
        linePoint2->y - linePoint->y,
        linePoint2->z - linePoint->z,
    };
    
    float dot = srgs_utility_vec_dot(planeNormal, &u);
    
    if (fabs(dot) > .00001) {
        srgs_vector3_t w = {
            linePoint->x - planePoint->x, 
            linePoint->y - planePoint->y, 
            linePoint->z - planePoint->z, 
        };
        
        float fac = -srgs_utility_vec_dot(planeNormal, &w) / dot;
        u.x *= fac;
        u.y *= fac;
        u.z *= fac;
        
        intersectResult->x = linePoint->x;
        intersectResult->y = linePoint->y;
        intersectResult->z = linePoint->z;
        
        intersectResult->x += u.x;
        intersectResult->y += u.y;
        intersectResult->z += u.z;
        
        return 1;
    }
    return 0;
}


void srgs_utility_vec_cross(srgs_vector3_t * dest, const srgs_vector3_t * a, const srgs_vector3_t * b) {
    dest->x = a->y * b->z - b->y * a->z;
    dest->y = a->z * b->x - b->z * a->x;
    dest->z = a->x * b->y - b->x * a->y;
}

float srgs_utility_vec_dot(const srgs_vector3_t * a, const srgs_vector3_t * b) {
    return
    a->x * b->x +
    a->y * b->y +
    a->z * b->z;
}


float srgs_utility_vec_theta(const srgs_vector3_t * a, const srgs_vector3_t * b) {
    return   acos((a->x*b->x + a->y*b->y + a->z*b->z) /
                  (sqrt(a->x*a->x + a->y*a->y + a->z*a->z) *
                   sqrt(b->x*b->x + b->y*b->y + b->z*b->z))) * (180 / M_PI);
    
}



float srgs_utility_vec_magnitude(const srgs_vector3_t * a) {
    return sqrt(a->x*a->x + a->y*a->y + a->z*a->z);
}


srgs_vector3_t srgs_utility_surface_normal(const srgs_vector3_t * p1, const srgs_vector3_t * p2, const srgs_vector3_t * p3) {
    srgs_vector3_t p21 = {
        p1->x - p2->x,
        p1->y - p2->y,
        p1->z - p2->z    
    };
    
    srgs_vector3_t p23 = {
        p3->x - p2->x,
        p3->y - p2->y,
        p3->z - p2->z    
    };

    srgs_vector3_t out;
    srgs_utility_vec_cross(&out, &p21, &p23);
    return out;
}




static float baseTranslationMatrix[16] = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

static float scratchMatrix[16];

static float baseRotationMatrix[16] = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

static float baseScaleMatrix[16] = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

static float baseOrthoMatrix[16] = {
    1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

static float basePerspMatrix[16] = {
    0.f, 0.f, 0.f, 0.f,
	0.f, 0.f, 0.f, 0.f,
	0.f, 0.f, 0.f, 0.f,
	0.f, 0.f,-1.f, 0.f
};

static float baseLookAtMatrix[16] = {
    1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};








void srgs_utility_matrix_print(srgs_matrix_t * m) {
	printf("Matrix %p:\n", m);

	printf("[%7.2f %7.2f %7.2f %7.2f]\n", m->x0, m->y0, m->z0, m->w0);
	printf("[%7.2f %7.2f %7.2f %7.2f]\n", m->x1, m->y1, m->z1, m->w1);
	printf("[%7.2f %7.2f %7.2f %7.2f]\n", m->x2, m->y2, m->z2, m->w2);
	printf("[%7.2f %7.2f %7.2f %7.2f]\n", m->x3, m->y3, m->z3, m->w3);
}

void srgs_utility_matrix_set(srgs_matrix_t * dest, srgs_matrix_t * src) {
	srgs_memcpy_float(dest, src, 16);
}

void srgs_utility_matrix_set_float(srgs_matrix_t * m,
	float x0, float y0, float z0, float w0,
	float x1, float y1, float z1, float w1,
	float x2, float y2, float z2, float w2,
	float x3, float y3, float z3, float w3
) {
	m->x0 = x0;
	m->y0 = y0;
	m->z0 = z0;
	m->w0 = w0;

	m->x1 = x1;
	m->y1 = y1;
	m->z1 = z1;
	m->w1 = w1;

	m->x2 = x2;
	m->y2 = y2;
	m->z2 = z2;
	m->w2 = w2;

	m->x3 = x3;
	m->y3 = y3;
	m->z3 = z3;
	m->w3 = w3;
}



void srgs_utility_matrix_set_identity(srgs_matrix_t * dest) {
	srgs_utility_matrix_set(dest, (srgs_matrix_t *)identityMatrix);
}

void srgs_utility_matrix_multiply(srgs_matrix_t * dest, const srgs_matrix_t * a_, const srgs_matrix_t * b_) {
	float * a = (float *)a_;
	float * b = (float *)b_;

	dest->x0 = a[0]*b[0] + a[1]*b[4] + a[2]*b[8] + a[3]*b[12];
	dest->y0 = a[0]*b[1] + a[1]*b[5] + a[2]*b[9] + a[3]*b[13];
	dest->z0 = a[0]*b[2] + a[1]*b[6] + a[2]*b[10] + a[3]*b[14];
	dest->w0 = a[0]*b[3] + a[1]*b[7] + a[2]*b[11] + a[3]*b[15];

	dest->x1 = a[4]*b[0] + a[5]*b[4] + a[6]*b[8] + a[7]*b[12];
	dest->y1 = a[4]*b[1] + a[5]*b[5] + a[6]*b[9] + a[7]*b[13];
	dest->z1 = a[4]*b[2] + a[5]*b[6] + a[6]*b[10] + a[7]*b[14];
	dest->w1 = a[4]*b[3] + a[5]*b[7] + a[6]*b[11] + a[7]*b[15];


	dest->x2 = a[8]*b[0] + a[9]*b[4] + a[10]*b[8] + a[11]*b[12];
	dest->y2 = a[8]*b[1] + a[9]*b[5] + a[10]*b[9] + a[11]*b[13];
	dest->z2 = a[8]*b[2] + a[9]*b[6] + a[10]*b[10] + a[11]*b[14];
	dest->w2 = a[8]*b[3] + a[9]*b[7] + a[10]*b[11] + a[11]*b[15];


	dest->x3 = a[12]*b[0] + a[13]*b[4] + a[14]*b[8] + a[15]*b[12];
	dest->y3 = a[12]*b[1] + a[13]*b[5] + a[14]*b[9] + a[15]*b[13];
	dest->z3 = a[12]*b[2] + a[13]*b[6] + a[14]*b[10] + a[15]*b[14];
	dest->w3 = a[12]*b[3] + a[13]*b[7] + a[14]*b[11] + a[15]*b[15];

}

srgs_matrix_t srgs_utility_matrix_multiply_(srgs_matrix_t * a, srgs_matrix_t * b) {
    srgs_matrix_t out;
    srgs_utility_matrix_multiply(&out, a, b);
    return out;
}

void srgs_utility_matrix_vector_multiply(srgs_vector4_t * dest, const srgs_matrix_t * matr_, const srgs_vector4_t * vec_) {
	float * matr = (float *) matr_;
	float * vec = (float *) vec_;

	dest->x = matr[0]*vec[0] 	+ matr[1]*vec[1]	+ matr[2]*vec[2]	+ matr[3]*vec[3];
	dest->y = matr[4]*vec[0] 	+ matr[5]*vec[1]	+ matr[6]*vec[2]	+ matr[7]*vec[3];
	dest->z = matr[8]*vec[0] 	+ matr[9]*vec[1]	+ matr[10]*vec[2]	+ matr[11]*vec[3];
	dest->w = matr[12]*vec[0] 	+ matr[13]*vec[1]	+ matr[14]*vec[2]	+ matr[15]*vec[3];
}





void srgs_utility_matrix_translate(srgs_matrix_t * transform, float x, float y, float z) {
	baseTranslationMatrix[3] = x;
	baseTranslationMatrix[7] = y;
	baseTranslationMatrix[11] = z;

	srgs_utility_matrix_set((srgs_matrix_t*)scratchMatrix, transform);



	srgs_utility_matrix_multiply(transform,
	           (srgs_matrix_t *) scratchMatrix,
	           (srgs_matrix_t *) baseTranslationMatrix
	);


}

void srgs_utility_matrix_scale(srgs_matrix_t * transform, float x, float y, float z) {
	baseScaleMatrix[0] = x;
	baseScaleMatrix[5] = y;
	baseScaleMatrix[10] = z;

	srgs_utility_matrix_set((srgs_matrix_t*)scratchMatrix, transform);
	srgs_utility_matrix_multiply(transform,
		(srgs_matrix_t*) scratchMatrix,
		(srgs_matrix_t*) baseScaleMatrix
	);
}


void srgs_utility_matrix_rotate(srgs_matrix_t * transform, float theta, float x, float y, float z) {
	// Implemented as specified by the opengl glRotate specification
	float len = sqrt(x*x + y*y + z*z);
	x = x/len;
	y = y/len;
	z = z/len;

	float c = cos((M_PI/180.0) *theta);
	float s = sin((M_PI/180.0) *theta);

	baseRotationMatrix[0] = x*x*(1 - c) + c;
	baseRotationMatrix[1] = x*y*(1 - c) - z*s;
	baseRotationMatrix[2] = x*z*(1 - c) + y*s;

	baseRotationMatrix[4] = y*x*(1 - c) + z*s;
	baseRotationMatrix[5] = y*y*(1 - c) + c;
    baseRotationMatrix[6] = y*z*(1 - c) - x*s;

    baseRotationMatrix[8] = x*z*(1 - c) - y*s;
    baseRotationMatrix[9] = y*z*(1 - c) + x*s;
    baseRotationMatrix[10] = z*z*(1 - c) + c;

    srgs_utility_matrix_set((srgs_matrix_t *) scratchMatrix, transform);

    srgs_utility_matrix_multiply(transform,
    	(srgs_matrix_t*)scratchMatrix,
    	(srgs_matrix_t*)baseRotationMatrix
    );


}


void srgs_utility_matrix_reverse_majority(srgs_matrix_t * transform) {
	float temp;

	temp = transform->x1; transform->x1 = transform->y0; transform->y0 = temp;
	temp = transform->x2; transform->x2 = transform->z0; transform->z0 = temp;
	temp = transform->y2; transform->y2 = transform->z1; transform->z1 = temp;
	temp = transform->x3; transform->x3 = transform->w0; transform->w0 = temp;
	temp = transform->y3; transform->y3 = transform->w1; transform->w1 = temp;
	temp = transform->z3; transform->z3 = transform->w2; transform->w2 = temp;
}

void srgs_utility_matrix_projection_orthographic(srgs_matrix_t * matrix, float left, float right, float bottom, float top, float near, float far) {
	//srgs_utility_matrix_SetIdentity(matrix);


	baseOrthoMatrix[0]  = (2 / (right - left));
	baseOrthoMatrix[5]  = (2 / (top - bottom));
	baseOrthoMatrix[10] = (-2 / (far - near));

	baseOrthoMatrix[3]  = - ((right + left) / (right - left));
	baseOrthoMatrix[7]  = - ((top + bottom) / (top - bottom));
	baseOrthoMatrix[11] = - ((far + near)   / (far - near));

    srgs_utility_matrix_set((srgs_matrix_t *) scratchMatrix, matrix);

    srgs_utility_matrix_multiply(matrix,
    	(srgs_matrix_t*)scratchMatrix,
    	(srgs_matrix_t*)baseOrthoMatrix
    );
}

// from GLU standard implementation notes
/*
void srgs_utility_matrix_projection_perspective(srgs_matrix_t * matrix, float fov, float aspect, float zNear, float zFar) {
    float f = cos((M_PI/180.0)*fov/2.0)/sin((M_PI/180.0)*fov/2.0);
    basePerspMatrix[0]  = f/aspect;
    basePerspMatrix[5]  = f;
    basePerspMatrix[10] =   (zFar+zNear) / (zNear - zFar);
    basePerspMatrix[11] = (2*zFar*zNear) / (zNear - zFar);
    basePerspMatrix[14] = -1;

    srgs_utility_matrix_set((srgs_matrix_t *) scratchMatrix, matrix);

    srgs_utility_matrix_multiply(matrix,
    	(srgs_matrix_t*)scratchMatrix,
    	(srgs_matrix_t*)basePerspMatrix
    );

}
*/


void srgs_utility_matrix_projection_perspective(srgs_matrix_t * m, float fovyInDegrees, float aspectRatio, float znear, float zfar) {
    float ymax, xmax;
    float temp, temp2, temp3, temp4;
    ymax = znear * tanf(fovyInDegrees * M_PI / 360.0);
    //ymin = -ymax;
    //xmin = -ymax * aspectRatio;
    xmax = ymax * aspectRatio;
    float * matrix = basePerspMatrix;
    float left   = -xmax;
    float right  = xmax;
    float bottom = -ymax; 
    float top    = ymax;
    
    temp = 2.0 * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;
    matrix[0] = temp / temp2;
    matrix[1] = 0.0;
    matrix[2] = 0.0;
    matrix[3] = 0.0;
    matrix[4] = 0.0;
    matrix[5] = temp / temp3;
    matrix[6] = 0.0;
    matrix[7] = 0.0;
    matrix[8] = (right + left) / temp2;
    matrix[9] = (top + bottom) / temp3;
    matrix[10] = (-zfar - znear) / temp4;
    matrix[11] = -1.0;
    matrix[12] = 0.0;
    matrix[13] = 0.0;
    matrix[14] = (-temp * zfar) / temp4;
    matrix[15] = 0.0;
    
    srgs_utility_matrix_reverse_majority((srgs_matrix_t*)basePerspMatrix);
    
    srgs_utility_matrix_set((srgs_matrix_t *) scratchMatrix, m);

    srgs_utility_matrix_multiply(m,
    	(srgs_matrix_t*)scratchMatrix,
    	(srgs_matrix_t*)basePerspMatrix
    );
    
}


// from GL standard implementation notes
void srgs_utility_matrix_look_at(srgs_matrix_t * matrix, const srgs_vector3_t * eye, const srgs_vector3_t * target, const srgs_vector3_t * up) {
    srgs_vector3_t f;
    f.x = target->x - eye->x;
    f.y = target->y - eye->y;
    f.z = target->z - eye->z;

    srgs_utility_vec_normalize(&f);





    // s = f X up2;
    srgs_vector3_t s;
    srgs_utility_vec_cross(&s, &f, up);
    srgs_utility_vec_normalize(&s);

    // u = normalized(s) x f
    srgs_vector3_t u;
    srgs_utility_vec_cross(&u, &s, &f);



    baseLookAtMatrix[0]  = s.x;
    baseLookAtMatrix[1]  = s.y;
    baseLookAtMatrix[2]  = s.z;

    baseLookAtMatrix[4]  = u.x;
    baseLookAtMatrix[5]  = u.y;
    baseLookAtMatrix[6]  = u.z;

    baseLookAtMatrix[8]  = -f.x;
    baseLookAtMatrix[9]  = -f.y;
    baseLookAtMatrix[10] = -f.z;


    srgs_utility_matrix_set((srgs_matrix_t *) scratchMatrix, matrix);

    srgs_utility_matrix_multiply(matrix,
    	(srgs_matrix_t*)scratchMatrix,
    	(srgs_matrix_t*)baseLookAtMatrix
    );

    srgs_utility_matrix_translate(matrix, -eye->x, -eye->y, -eye->z);

}


srgs_matrix_t * srgs_utility_matrix_get_identity() {
	return (srgs_matrix_t *)identityMatrix;
}
// From MESA's glu implementation
void srgs_utility_matrix_invert(srgs_matrix_t * matr) {
    srgs_utility_matrix_reverse_majority(matr);

    float inv[16], det, invOut[16];
    float * m = (float*)matr;
    int i;

    inv[ 0] =  m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[ 4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[ 8] =  m[4] * m[ 9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[ 9];
    inv[12] = -m[4] * m[ 9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[ 9];
    inv[ 1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[ 5] =  m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[ 9] = -m[0] * m[ 9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[ 9];
    inv[13] =  m[0] * m[ 9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[ 9];
    inv[ 2] =  m[1] * m[ 6] * m[15] - m[1] * m[ 7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[ 7] - m[13] * m[3] * m[ 6];
    inv[ 6] = -m[0] * m[ 6] * m[15] + m[0] * m[ 7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[ 7] + m[12] * m[3] * m[ 6];
    inv[10] =  m[0] * m[ 5] * m[15] - m[0] * m[ 7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[ 7] - m[12] * m[3] * m[ 5];
    inv[14] = -m[0] * m[ 5] * m[14] + m[0] * m[ 6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[ 6] + m[12] * m[2] * m[ 5];
    inv[ 3] = -m[1] * m[ 6] * m[11] + m[1] * m[ 7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[ 9] * m[2] * m[ 7] + m[ 9] * m[3] * m[ 6];
    inv[ 7] =  m[0] * m[ 6] * m[11] - m[0] * m[ 7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[ 8] * m[2] * m[ 7] - m[ 8] * m[3] * m[ 6];
    inv[11] = -m[0] * m[ 5] * m[11] + m[0] * m[ 7] * m[ 9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[ 9] - m[ 8] * m[1] * m[ 7] + m[ 8] * m[3] * m[ 5];
    inv[15] =  m[0] * m[ 5] * m[10] - m[0] * m[ 6] * m[ 9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[ 9] + m[ 8] * m[1] * m[ 6] - m[ 8] * m[2] * m[ 5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];



    det = 1.f / det;

    for(i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;
    srgs_memcpy_float(matr, invOut, 16);
    srgs_utility_matrix_reverse_majority(matr);
}

srgs_vector3_t srgs_utility_matrix_transform(const srgs_matrix_t * m, const srgs_vector3_t * v) {
    srgs_vector4_t out;
    srgs_vector4_t in = {v->x, v->y, v->z, 1.f};
    srgs_utility_matrix_vector_multiply(&out, (srgs_matrix_t*)m, &in);
    srgs_vector3_t o = {out.x, out.y, out.z};
    return o;
}



static void srgs_utility_matrix_transform_inplace(const srgs_matrix_t * m, srgs_vector3_t * v) {
	const float * matr = (float *) m;

	float f0 = matr[0] *v->x + matr[1] *v->y + matr[2]*v->z  + matr[3];
	float f1 = matr[4] *v->x + matr[5] *v->y + matr[6]*v->z  + matr[7];
	float f2 = matr[8] *v->x + matr[9] *v->y + matr[10]*v->z + matr[11];
    float f3 = matr[12]*v->x + matr[13]*v->y + matr[14]*v->z + matr[15];
    v->x = f0 / f3;
    v->y = f1 / f3;
    v->z = f2 / f3;
    
}




static volatile float min3(float x, float y, float z) {
    if (x < y) {
        if (x < z)
            return x;
        else
            return z;
    } else {
        if (y < z)
            return y;
        else 
            return z;
    }
}


static volatile float max3(float x, float y, float z) {
    if (x > y) {
        if (x > z)
            return x;
        else
            return z;
    } else {
        if (y > z)
            return y;
        else 
            return z;
    }
}



static int depth_l(uint32_t current, uint32_t in) {
    return in < current;
}

static int depth_lte(uint32_t current, uint32_t in) {
    return in <= current;
}

static int depth_eq(uint32_t current, uint32_t in) {
    return in == current;
}

static int depth_gt(uint32_t current, uint32_t in) {
    return in > current;
}

static int depth_gte(uint32_t current, uint32_t in) {
    return in >= current;
}


static int depth_always(uint32_t current, uint32_t in) {
    return 1;
}

static int depth_never(uint32_t current, uint32_t in) {
    return 0;
}


static void srgs_render__renderlist(
    srgs_t * s, 
    srgs_renderlist_t * list, 
    srgs_texture_t * framebuffer, 
    srgs_texture_t * depthbuffer
)  {
    


    uint32_t i, n;
    srgs_object_t * obj;
    srgs_texture_t * texture;

    // for any matrix, if set to ID 0, this is reserved as the 
    // "identity matrix", so calculation is skipped if its 
    // set to zero.
    srgs_matrix_t * objTF = NULL;
    srgs_matrix_t * viewTF = NULL;
    srgs_matrix_t * projTF = NULL;
    srgs_matrix_t tf, tf2;


    // triangles to transform and rasterize
    uint32_t triCount, fetch;    

    // vertices to be transformed. Forms one triangle
    srgs_vector3_t pos[3];

    // barycentric transform and utility data
    float barycentricTF[4];
    float barycentricDet;
    float barycentricInv[4];
    float barycentricCartVx[3];
    float barycentricCartVy[3];


    float bias0,
          bias1,
          bias2,

          xdiff,
          ydiff,

          convz,
          * color0,
          * color1,
          * color2,

          * uv0,
          * uv1,
          * uv2,

          uvx,
          uvy;

    // bounding and iterators
    int boundXmin,
        boundXmax,
        boundYmin,
        boundYmax,
        x, y,
        framebufferW, framebufferH,
        fetchx, fetchy;

    
    
    uint32_t fragDepth, *fragP;


    int (*depthTest)(uint32_t, uint32_t);




    framebufferW = framebuffer->w;
    framebufferH = framebuffer->h;


    

    projTF = id_table_fetch(s->matrices, srgs_matrix_t, list->projID);
    viewTF = id_table_fetch(s->matrices, srgs_matrix_t, list->viewID);
    


    for(i = 0; i < list->size; ++i) {
        obj = id_table_fetch(s->objects, srgs_object_t, list->objects[i]);
        // choose depth test

        switch(obj->depthMode) {
          case srgs__object_depth_mode__less:         depthTest = depth_l; break;
          case srgs__object_depth_mode__lessequal:    depthTest = depth_lte; break;
          case srgs__object_depth_mode__equal:        depthTest = depth_eq; break;
          case srgs__object_depth_mode__greater:      depthTest = depth_gt; break;
          case srgs__object_depth_mode__greaterequal: depthTest = depth_gte; break;
          case srgs__object_depth_mode__always:       depthTest = depth_always; break;
          case srgs__object_depth_mode__never:        depthTest = depth_never; break;
        }



        // skip object entirely.
        if (depthTest == depth_never) continue;
        

        //if (obj->matrixID) {
            objTF = id_table_fetch(s->matrices, srgs_matrix_t, obj->matrixID);
        //}


        triCount = obj->indexCount/3;
        for(n = 0; n < triCount; ++n) {

            pos[0] = *((srgs_vector3_t*)(obj->verticesInterleaved+SRGS__FLOATS_PER_VERTEX*obj->indices[n*3+0]));
            pos[1] = *((srgs_vector3_t*)(obj->verticesInterleaved+SRGS__FLOATS_PER_VERTEX*obj->indices[n*3+1]));
            pos[2] = *((srgs_vector3_t*)(obj->verticesInterleaved+SRGS__FLOATS_PER_VERTEX*obj->indices[n*3+2]));


            /// TODO make efficient
            srgs_utility_matrix_multiply(&tf,  projTF, viewTF);
            srgs_utility_matrix_multiply(&tf2, &tf, objTF);
            //tf2 = tf;

            srgs_utility_matrix_transform_inplace(&tf2, pos);
            srgs_utility_matrix_transform_inplace(&tf2, pos+1);
            srgs_utility_matrix_transform_inplace(&tf2, pos+2);


            // prepare barycentric transform.
            // this determines which fragments are within the object.
            barycentricCartVx[0] = framebufferW*(pos[0].x+1)/2.f;
            barycentricCartVy[0] = framebufferH*(pos[0].y+1)/2.f;

            barycentricCartVx[1] = framebufferW*(pos[1].x+1)/2.f;
            barycentricCartVy[1] = framebufferH*(pos[1].y+1)/2.f;

            barycentricCartVx[2] = framebufferW*(pos[2].x+1)/2.f;
            barycentricCartVy[2] = framebufferH*(pos[2].y+1)/2.f;


            barycentricTF[0] = (barycentricCartVx[0] - barycentricCartVx[2]);
            barycentricTF[1] = (barycentricCartVx[1] - barycentricCartVx[2]);
            barycentricTF[2] = (barycentricCartVy[0] - barycentricCartVy[2]);
            barycentricTF[3] = (barycentricCartVy[1] - barycentricCartVy[2]);

            barycentricDet = 1/(barycentricTF[0]*barycentricTF[3] -
                                barycentricTF[1]*barycentricTF[2]);

            barycentricInv[0] = barycentricTF[3] * barycentricDet;
            barycentricInv[1] =-barycentricTF[1] * barycentricDet;
            barycentricInv[2] =-barycentricTF[2] * barycentricDet;
            barycentricInv[3] = barycentricTF[0] * barycentricDet;


            


            // upper and lower bounds to restrict which fragments to test
            // most optimization should go here
            boundXmin = framebufferW * (min3(pos[0].x, pos[1].x, pos[2].x)+1)/2.f;
            boundYmin = framebufferH * (min3(pos[0].y, pos[1].y, pos[2].y)+1)/2.f;
            boundXmax = framebufferW * (max3(pos[0].x, pos[1].x, pos[2].x)+1)/2.f;
            boundYmax = framebufferH * (max3(pos[0].y, pos[1].y, pos[2].y)+1)/2.f;


            if (boundXmin < 0) boundXmin = 0;
            if (boundXmax >= framebufferW) boundXmax = framebufferW-1;
            if (boundYmin < 0) boundYmin = 0;
            if (boundYmax >= framebufferH) boundYmax = framebufferH-1;


            for(y = boundYmin; y <= boundYmax; ++y) {
                for(x = boundXmin; x <= boundXmax; ++x) {
                    
                    // use the barycentric transform to get "biases" toward  
                    // each vertex. This determines how to blend colors and 
                    // UVs.

                    xdiff = x - barycentricCartVx[2];
                    ydiff = y - barycentricCartVy[2];

                    bias0 = barycentricInv[0]*xdiff + barycentricInv[1]*ydiff;
                    bias1 = barycentricInv[2]*xdiff + barycentricInv[3]*ydiff;
                    bias2 = 1 - bias0 - bias1;


                    
                    if (bias0 < 0 || bias1 < 0 || bias2 < 0) continue;



                    // next check to see if it passes the depth test
                    convz = bias0 * pos[0].z +
                            bias1 * pos[1].z +
                            bias2 * pos[2].z;

                    // near/far clipping
                    if (convz < -1.f || convz > 1.0) continue;

                    fragDepth = ((convz + 1.f)/2.f) * 0xffffffff;
                    uint32_t fragment = (x + y*depthbuffer->w)*4;
                    fragP = (uint32_t*)(depthbuffer->data+fragment);
                    if (depthTest(*fragP, fragDepth)) {

                        // update depth buffer (1st byte only)
                        *fragP = fragDepth;



                        // TODO: ALPHA BLENDING mode
                        switch(obj->renderMode) {

                          // replace with incoming color
                          case srgs__object_render_mode__color:
                            color0 = ((float*)obj->verticesInterleaved+SRGS__FLOATS_PER_VERTEX*obj->indices[n*3+0])+5;
                            color1 = ((float*)obj->verticesInterleaved+SRGS__FLOATS_PER_VERTEX*obj->indices[n*3+1])+5;
                            color2 = ((float*)obj->verticesInterleaved+SRGS__FLOATS_PER_VERTEX*obj->indices[n*3+2])+5;

                            framebuffer->data[fragment  ] = (bias0*color0[0] + bias1*color1[0] + bias2*color2[0])*0xff;
                            framebuffer->data[fragment+1] = (bias0*color0[1] + bias1*color1[1] + bias2*color2[1])*0xff;
                            framebuffer->data[fragment+2] = (bias0*color0[2] + bias1*color1[2] + bias2*color2[2])*0xff;
                            framebuffer->data[fragment+3] = (bias0*color0[3] + bias1*color1[3] + bias2*color2[3])*0xff;
                            break;

                          // replace with incoming texel fetch
                          case srgs__object_render_mode__texture: 
                            uv0 = ((float*)obj->verticesInterleaved+SRGS__FLOATS_PER_VERTEX*obj->indices[n*3+0])+3;
                            uv1 = ((float*)obj->verticesInterleaved+SRGS__FLOATS_PER_VERTEX*obj->indices[n*3+1])+3;
                            uv2 = ((float*)obj->verticesInterleaved+SRGS__FLOATS_PER_VERTEX*obj->indices[n*3+2])+3;

                            uvx = bias0*uv0[0] + bias1*uv1[0] + bias2*uv2[0];
                            uvy = bias0*uv0[1] + bias1*uv1[1] + bias2*uv2[1];
    
                            if (uvx < 0) uvx = 0;
                            if (uvx > 1) uvx = 1;
                            if (uvy < 0) uvy = 0;
                            if (uvy > 1) uvy = 1;

                            texture = id_table_fetch(s->textures, srgs_texture_t, obj->texture);
                            fetchx = round((texture->w-1)*uvx);
                            fetchy = round((texture->h-1)*uvy);
                            fetch = (fetchx + fetchy*texture->w)*4;                        

                            *((int*)(framebuffer->data+fragment)) = *((int*)(texture->data+fetch));  
                            break;


                          // no color, just depth buffer
                          case srgs__object_render_mode__depth_only:
                            break;

                        }
                    }
                }
            }
        }
    }
}



srgs__render_error srgs_render(srgs_t * s, uint32_t count, uint32_t * renderListIDs) {
    uint32_t i;
    srgs_texture_t * framebuffer = id_table_fetch(s->textures, srgs_texture_t, s->framebufferID);
    srgs_texture_t * depthbuffer = id_table_fetch(s->textures, srgs_texture_t, s->depthbufferID);


    if (framebuffer->w != depthbuffer->w ||
        framebuffer->h != depthbuffer->h) {
        return srgs__render_error__framebuffer_mismatch;
    }

    for(i = 0; i < count; ++i) {
        srgs_render__renderlist(
            s, 
            id_table_fetch(
                s->renderLists, 
                srgs_renderlist_t, 
                renderListIDs[i]
            ),
            framebuffer,
            depthbuffer
        );
    }
    return srgs__render_error__none;
}













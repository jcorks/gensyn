#include <gensyn/ring.h>

#include <stdlib.h>
#include <string.h>

#ifdef GENSYNDC_DEBUG
#include <assert.h>
#endif


struct gensyn_ring_t {
    uint32_t read;
    uint32_t write;
    
    uint8_t * buffer;
    uint32_t sizeofType;
    uint32_t count;
};



gensyn_ring_t * gensyn_ring_create(uint32_t sizeofType, uint32_t count) {
    gensyn_ring_t * ring = calloc(1, sizeof(gensyn_ring_t));
    ring->sizeofType = sizeofType;
    ring->buffer = calloc(sizeofType, count);
    ring->count = count;
    return ring;
}


void gensyn_ring_destroy(gensyn_ring_t * r) {
    free(r->buffer);
    free(r);
}



int gensyn_ring_has_pending(const gensyn_ring_t * r) {
    return r->read != r->write;
}

int gensyn_ring_push_p(gensyn_ring_t * r, const void * p) {
    uint32_t writeReal = r->write;
    if (writeReal == (r->read-1)%r->count) return 0;
    if (writeReal == (r->read-2)%r->count) return 0; // read-ahead buffer

    memcpy(
        r->buffer + writeReal * (r->sizeofType*writeReal),
        p,
        r->sizeofType
    );
    r->write = (writeReal+1)%r->count;
    return 1;
}

const void * gensyn_ring_pop_p(gensyn_ring_t * r) {
    const void * out = r->buffer+r->read*r->sizeofType;
    r->read++;
    return out;
}




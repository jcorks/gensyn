#ifndef H_GENSYNDC__RING__INCLUDED
#define H_GENSYNDC__RING__INCLUDED


#include <stdint.h>

/*

    RingBuffer
    -----

    Statically sized buffer which can be used 
    to communicate between multiple threads
    

*/
typedef struct gensyn_ring_t gensyn_ring_t;



/// Returns a new, empty array 
///
/// sizeofType refers to the size of the elements that the array will hold 
/// the most convenient way to do this is to use "sizeof()"
gensyn_ring_t * gensyn_ring_create(uint32_t sizeofType, uint32_t count);

/// Destroys the container and buffer that it manages.
///
void gensyn_ring_destroy(gensyn_ring_t *);

// returns whether there is data pending to be read
int gensyn_ring_has_pending(const gensyn_ring_t *);


// pushes new data.
int gensyn_ring_push_p(gensyn_ring_t *, const void * p);
#define gensyn_ring_push(__G__, __P__) (gensyn_ring_push_p(__G__, &(__P__)));

// can only pop when has_pending returns 1
const void * gensyn_ring_pop_p(gensyn_ring_t *);
#define gensyn_ring_pop(__G__, __T__) (*((__T__*)gensyn_ring_pop_p(__G__)));


#endif

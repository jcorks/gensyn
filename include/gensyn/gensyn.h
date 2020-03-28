#ifndef H_GENSYN_CONTEXT__INCLUDED
#define H_GENSYN_CONTEXT__INCLUDED

#include <gensyn/string.h>
typedef struct gensyn_gate_t gensyn_gate_t;


typedef struct gensyn_t gensyn_t;





gensyn_t * gensyn_create();


// Gets the main output gate.
gensyn_t * gensyn_get_output_gate(const gensyn_t *);




// Runs a command through the command processor.
//
const gensyn_string_t * gensyn_send_command(const gensyn_t *, const gensyn_string_t *);



// Creates a new gate associated with a name.
// When done, you should remove it with gensyn_destroy_named_gate.
gensyn_gate_t * gensyn_create_named_gate(
    gensyn_t *, 
    const gensyn_string_t * type,
    const gensyn_string_t * name
);


// Returns a gate with the given name. If none exists, NULL is returned.
gensyn_gate_t * gensyn_get_named_gate(const gensyn_t *, const gensyn_string_t *);


// Destroys and cleans up a named gate. This should only be used for named gates.
void gensyn_destroy_named_gate(const gensyn_t *, const gensyn_string_t *);




// Sets the origin for the rendered scene.
void gensyn_set_origin(gensyn_t *, int x, int y);

// Renders the scene
void gensyn_render_scene(gensyn_t *);

// Gets a handle to the framebuffer texture.
int gensyn_get_framebuffer_texture(gensyn_t *);


#endif

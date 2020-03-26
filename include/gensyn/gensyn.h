#ifndef H_GENSYN_CONTEXT__INCLUDED
#define H_GENSYN_CONTEXT__INCLUDED

typedef struct gensyn_t gensyn_t;





gensyn_t * gensyn_create();


// Gets the main output gate.
gensyn_t * gensyn_get_output_gate(const gensyn_t *);




// Sets the origin for the rendered scene.
void gensyn_set_origin(gensyn_t *, int x, int y);

// Renders the scene
void gensyn_render_scene(gensyn_t *);

// Gets a handle to the framebuffer texture.
int gensyn_get_framebuffer_texture(gensyn_t *);


#endif

#ifndef H__SRGS___INCLUDED
#define H__SRGS___INCLUDED

#include <stdint.h>
#include <string.h>

/*
                                 SRGS
                                 ====
                    Simple Raster Graphics Server
                    -----------------------------
                        Johnathan Corkery, 2020


    A simple implementation of baseline behavior expected of a 
    bare-bones graphics context. This is meant to be a 
    simple starting point / option for low-resolution graphics 
    in contexts where more advanced libraries tied to hardware 
    (i.e. OpenGL / DirectX) are not available.

    SRGS is meant to be a single-file implementation of graphics 
    features that would normally be controlled by dedicated 
    hardware. This API is designed to support hardware-accelerated
    implementations, but such a thing is not required.

    As it is, this library is purely a software rasterizer that is 
    limited to only color and depth information with ONLY triangle
    primitives. 

    The reason that the term "server" is used is due to the API 
    organization. Note that no pointers are used outside of the 
    context pointer and array buffers for bulk data (like 
    vertex data), so an implementation with deferred processing
    of graphics requests is easily achievable, with perhaps 
    the graphics processor being not physically attached to the 
    calling machine.


    General notes:
    --------------
    - The origin is the "topleft" at 0, 0
    - Once delivered to the renderer, coordinates will be 
      interpreted as homogeneous coordinates. That is,
      coordiates within [-1, 1] for xyz.
    - All pixel/texel information is in RGBA format, where each 
       component is 1 byte.
    - The depth buffer is 4 bytes.
    - Only one primitive is available: triangle.

    - No face culling is implemented. Yet.

    - Linear algebra utilities are available if needed.



*/





// Enumerator for object rendering modes.
// This controls how triangles are colored.
// see srgs_object_set_render_mode()
typedef enum {
    // Specifies that each triangle should be colored based on 
    // each vertex color. The vertex color is specified using the 
    // 4-component color component
    // This is the default.
    srgs__object_render_mode__color,

    // Specifies that each triangle should be colored based on 
    // the associated texture. See srgs_object_set_texture()
    // When active, the object's UVs are used to determine which 
    // fragments receive which texels.
    srgs__object_render_mode__texture,

    // Specifies that no coloring will be done of the tringle.
    // Depth information will still be processed in this case.
    srgs__object_render_mode__depth_only
} srgs__object_render_mode;






// Enumerator for controlling depth testing.
// The depth test is the part of the graphics pipeline that 
// determines which fragments will be shown on-screen.
// see srgs_object_set_depth_mode()
typedef enum {
    // If a fragement's depth is less (that is, close to the observor)
    // then the fragments are shown.
    srgs__object_depth_mode__less,

    // If a fragment's depth is less than or equal to the existing 
    // fragment's depth, then it is shown.
    srgs__object_depth_mode__lessequal,

    // If a fragment's depth is equal to the existing 
    // fragment's depth, then it is shown.
    srgs__object_depth_mode__equal,

    // If a fragment's depth is greater than the existing 
    // fragment's depth, then it is shown.
    srgs__object_depth_mode__greater,

    // If a fragment's depth is greater than or equal to the existing 
    // fragment's depth, then it is shown.
    srgs__object_depth_mode__greaterequal,

    // The fragment always succeeds its depth test. 
    srgs__object_depth_mode__always,

    // The fragment never succeeds. Effectively disables the object.
    srgs__object_depth_mode__never
} srgs__object_depth_mode;



// Enumerator for which channel to work with of an object's
// vertices. Each vertex contains the following information:
//      position: x, y, z position
//      UVs:      u, v texture coordinates
//      color:    r, g, b, z color components
typedef enum {
    // The x, y, z of the vertex.
    srgs__object_vertex_channel__position,

    // The u, v of the vertex.
    srgs__object_vertex_channel__uvs,

    // The r, g, b, a of the vertex.
    srgs__object_vertex_channel__color,

} srgs__object_vertex_channel;





typedef enum {
    srgs__render_error__none,

    // THe dimensions of the framebuffer and 
    // depth buffer do not match
    srgs__render_error__framebuffer_mismatch,

} srgs__render_error;






// The context for the server.
// Required for all calls.
typedef struct srgs_t srgs_t;



/* 
    SRGS context API 

    The context for all renderer calls.
*/

// Creates a new context.
// startH/W is the starting pixel with and height for the 
// framebuffer and depthbuffer.
//
// If you wish, custom allocator functions may 
// be specified.
srgs_t * srgs_create(
    uint32_t startW, uint32_t startH,

    // can be NULL. In that case, malloc() is used.
    void * (*allocatorFunction)  (size_t),

    // can be NULL. In that case, free() is used.
    void   (*deallocatorFunction)(void *),

    // can be NULL. In that case, realloc() is used.
    void * (*reallocatorFunction)(void *, size_t)


);

// Frees the SRGS context.
void srgs_destroy(srgs_t *);


// Returns the texture ID for the framebuffer.
// Always defined and never changes across the context's lifetime.
// Can be used as a normal texture.
uint32_t srgs_get_framebuffer_texture(const srgs_t *);

// Returns the texture ID for the depthbuffer.
// Always defined and never changes across the context's lifetime.
// Can be used as a normal texture.
// For the depth buffer, only the red component is used.
// This means that the depth buffer bitspace is only one byte.
uint32_t srgs_get_depthbuffer_texture(const srgs_t *);


// Resets the depth buffer. This is an alias for 
// doing a texture blank with the value 0xff.
// For the default (and standard) "Less" test that objects use, 
// this is recommended.
void srgs_clear_depth(srgs_t *);

// Resets the color buffer. This is an alias for 
// doing a texture blank with all black and 0 for the alpha channel
void srgs_clear_color(srgs_t *);


// Renders all given renderlists. The results are stored within the 
// framebuffer and the depthbuffer is used for depth processing in 
// its current state.
srgs__render_error srgs_render(srgs_t *, uint32_t count, uint32_t * renderListIDs);







/* 
    SRGS Texture API 

    Textures are matrices of pixels (referred to as texels)
    that can be used as sources for coloring triangles. This 
    is the main way to import picture information in a 
    graphical context.    
    
*/

// Creates a new texture within the context.
// w and h are the height and width in texels 
// that the texture should start at in size.
// The ID for the texture is use.
uint32_t srgs_texture_create(srgs_t *, uint32_t w, uint32_t h);

// Returns whether the given ID actually points 
// to a valid texture. If it is valid, 1 is returned.
// else 0 is returned.
// This is appropriate for debugging context, but should not 
// be relied on too heavily elsewise since it incurs a noteable 
// runtime cost for no programmatic benefit unless 
// untrusted sources are at play.
int srgs_texture_verify(srgs_t *, uint32_t id);

// Destroys and frees all resources associated with the texture
// handle.
void srgs_texture_destroy(srgs_t *, uint32_t id);

// Resets the texture to a gray state. Each component of the
// texture will match the given color value.
void srgs_texture_blank(srgs_t *, uint32_t, uint8_t component);


// Updates the texture.
// Using an RGBA buffer (src), the target texture's data may be 
// replaced. 
// x/yDest is the x,y position of the texutre to edit. This is relative 
// to the origin. x/ySrc is the origin of the incoming texture, and 
// w/hSrc is the width and height of the incoming texture.
void srgs_texture_update(
    srgs_t *, 
    uint32_t ID,

    const uint8_t * src, 
    uint32_t xDest, uint32_t yDest, 
    uint32_t xSrc,  uint32_t ySrc,
    uint32_t wSrc,  uint32_t hSrc
);

// Resizes the texture to the given width and height. The 
// contents of the texture are undefined after this operation.
void srgs_texture_resize(srgs_t *, uint32_t id, uint32_t w, uint32_t h);

// Retrieves a pointer to the raw texture data. The texture data is 
// always in RGBA format where each component is one byte of information.
const uint8_t * srgs_texture_get_data(const srgs_t *, uint32_t);

// Returns the width of the texture in texels.
uint32_t srgs_texture_get_width(const srgs_t *, uint32_t);

// Returns the height of the texture in texels.
uint32_t srgs_texture_get_height(const srgs_t *, uint32_t);








/* 
    SRGS Matrix API 
    
    Matrices define arbitrary transforms.
    They can be specified for objects and renderlists.
*/



// Basic struct for a matrix.
// Exposed for easy access, but is effectively a 16-component 
// array of floats.
typedef struct {
	  float x0, y0, z0, w0,
			x1, y1, z1, w1,
			x2, y2, z2, w2,
			x3, y3, z3, w3;
} srgs_matrix_t;


// Creates a new matrix and returns its ID.
// Every new matrix is set to the identity matrix.
uint32_t srgs_matrix_create(srgs_t *);


// Returns whether the given ID actually points 
// to a valid matrix. If it is valid, 1 is returned.
// else 0 is returned.
// This is appropriate for debugging context, but should not 
// be relied on too heavily elsewise since it incurs a noteable 
// runtime cost for no programmatic benefit unless 
// untrusted sources are at play.
int srgs_matrix_verify(srgs_t *, uint32_t id);

// Frees all data associated with a matrix.
void srgs_matrix_destroy(srgs_t *, uint32_t id);


// Sets the data for a matrix.
void srgs_matrix_set(srgs_t *, uint32_t id, srgs_matrix_t *);

// Gets the raw data for a matrix.
const srgs_matrix_t * srgs_matrix_get(const srgs_t *, uint32_t id);










/* 
    SRGS Object API 

    An object is the atomic unit by which rendering takes place.
    Each object is a collection of vertices expressed with exactly 
    one texture and one transform.

    If the channel is position: the float buffer is x, y, z per vertex.
    If the channel is uvs: the float buffer is u, v per vertex.
    If the channel is position: the float buffer is r, g, b, a per vertex.
    
*/

uint32_t srgs_object_create(srgs_t *);

// Returns whether the given ID actually points 
// to a valid object. If it is valid, 1 is returned.
// else 0 is returned.
// This is appropriate for debugging context, but should not 
// be relied on too heavily elsewise since it incurs a noteable 
// runtime cost for no programmatic benefit unless 
// untrusted sources are at play.
int srgs_object_verify(srgs_t *, uint32_t id);

// Creates a new object as a clone of an existing one.
uint32_t srgs_object_clone(srgs_t *, uint32_t id);

// Frees an object instance.
void srgs_object_destroy(srgs_t *, uint32_t id);

// Sets the vertex count for the object.
// Once set, defining and updating verices will 
// assume this is the maximum number of vertices for the object.
void srgs_object_set_vertex_count(srgs_t *, uint32_t id, uint32_t vertexCount);

// Redefines the vertex buffer in the given channel.
// Assumes the incoming buffer matches in size to the actual buffer.
void srgs_object_define_vertices(
    srgs_t *, 
    uint32_t ID, 
    srgs__object_vertex_channel, 
    float *
);

// Updates the given object's vertex buffer channel
void srgs_object_update_vertices(
    srgs_t *, 
    uint32_t ID, 
    srgs__object_vertex_channel, 
    uint32_t fromVertexIndex, 
    uint32_t toVertexIndex, 
    float * src
);

// Retrieves the full vertex buffer for the 
// object. The buffer is in interleaved format for all 
// the given channels. Each vertex is organized in 
// the following format:
//
//  x,y,z     u,v,     r,g,b,a
// 
const float * srgs_object_get_vertices(
    const srgs_t *, 
    int32_t ID
);


// Defines the actual indices of the object
// These determine which vertices defined are used to render 
// and in what order. For example,
// specifying to render indices "2, 3, and 8" will 
// fetch vertices 2, 3, and 8 in that for when determining 
// what to render.
//
// If there are fewer than 3 vertices leftover at the end of 
// the buffer, these are referred to as "degenerate vertices".
// These are ignored in this renderer since triangles 
// are the only primitive.
void srgs_object_define_indices(
    srgs_t *, 
    uint32_t ID, 
    uint32_t count, 
    uint32_t * indices
);

// Sets the render mode for the object.
// See srgs__object_render_mode
void srgs_object_set_render_mode(srgs_t *, uint32_t ID, srgs__object_render_mode);

// Sets the depth mode/test for the object.
// See srgs__object_depth_mode
void srgs_object_set_depth_mode(srgs_t *, uint32_t ID, srgs__object_depth_mode);

// Sets the texture to be used with the object.
void srgs_object_set_texture(srgs_t *, uint32_t ID, uint32_t);

// Sets the transform matrix to be used with this object.
void srgs_object_set_transform(srgs_t *, uint32_t ID, uint32_t);


// Retrieves parameters for the object.
// any unneeded attribute can be set NULL and is ignored.
void srgs_object_get_parameters(
    const srgs_t *,
    uint32_t ID,
    
    srgs__object_render_mode *,
    srgs__object_depth_mode *,
    uint32_t * matrixID,
    uint32_t * textureID,
    uint32_t * vertexCount
);






/*
    SRGS Renderlist API


    Renderlists are groupings of Objects 
    and can be given an overlying transform.

*/ 


uint32_t srgs_renderlist_create(srgs_t *);

// Returns whether the given ID actually points 
// to a valid renderlist. If it is valid, 1 is returned.
// else 0 is returned.
// This is appropriate for debugging context, but should not 
// be relied on too heavily elsewise since it incurs a noteable 
// runtime cost for no programmatic benefit unless 
// untrusted sources are at play.
int srgs_renderlist_verify(srgs_t *, uint32_t);


// Destroys and frees a renderlist instance.
void srgs_renderlist_destroy(srgs_t *, uint32_t);

// Sets the viewing transform for objects that it renders.
// The transforms are applied in this order:
//
// projectionTransform * viewTransform * objectTransform;
//
void srgs_renderlist_set_view_transform(srgs_t *, uint32_t, uint32_t);

// Sets the projection transform for objects that it renders.
// The transforms are applied in this order:
//
// projectionTransform * viewTransform * objectTransform;
//
void srgs_renderlist_set_projection_transform(srgs_t *, uint32_t, uint32_t);


// Sets the objects that this renderlist contains.
void srgs_renderlist_set_objects(
    srgs_t *, 
    uint32_t list, 
    uint32_t count, 
    uint32_t * objectIDs
);
























/* Vector utilities */


typedef struct {
    float x; float y; float z;
} srgs_vector3_t;

typedef struct {
    float x; float y; float z; float w;
} srgs_vector4_t;

typedef struct {
    float x; float y;
} srgs_vector2_t;





// Allocates a 4x1 matrix set to the identity -> (0, 0, 0, 1)
srgs_vector4_t * srgs_utility_new_vec4();

// Convenience constructors for platforms that dont support input static definition.
srgs_vector3_t SRGSVEC3(float x, float y, float z);
srgs_vector4_t SRGSVEC4(float x, float y, float z, float w);

// Sets a given vector to the given values
void srgs_utility_vec_set(srgs_vector4_t * vec, float x, float y, float z, float w);


// Sets dest to a X b
void srgs_utility_vec_cross(srgs_vector3_t * dest, const srgs_vector3_t * a, const srgs_vector3_t * b);

// Returns a (dot) b
float srgs_utility_vec_dot(const srgs_vector3_t * a, const srgs_vector3_t * b);


// returns the angle between 2 vectors in degrees
float srgs_utility_vec_theta(const srgs_vector3_t * a, const srgs_vector3_t * b);

// performs the dot product:  a . b
float srgs_utility_vec_dot(const srgs_vector3_t * a, const srgs_vector3_t * b);

// Returns the distance between a and b as positional vectors
float srgs_utility_vec_distance(const srgs_vector3_t * a, const srgs_vector3_t * b);

// convenience function for normalize((p1 - p2) x (p3 - p2))
srgs_vector3_t srgs_utility_surface_normal(const srgs_vector3_t * p1, const srgs_vector3_t * p2, const srgs_vector3_t * p3);


// Performs a "intersection test". Returns whether the line formed by 
// linePoint and linePoint2 intersects the surface defined by pl
int srgs_utility_intersection_test(
    const srgs_vector3_t * linePoint,
    const srgs_vector3_t * linePoint2,
    
    const srgs_vector3_t * surfaceP1,
    const srgs_vector3_t * surfaceP2,
    const srgs_vector3_t * surfaceP3,
    
    srgs_vector3_t * intersectResult
);

// Same as intersection_test except:
// - A line segment is specified rather than a ray 
// - The plane is defined in position-normal form. 
int srgs_utility_intersection_test_plane(
    const srgs_vector3_t * linePoint,
    const srgs_vector3_t * linePoint2,
    
    const srgs_vector3_t * planePoint,
    const srgs_vector3_t * planeNormal,
    
    srgs_vector3_t * intersectResult
);

// Retruns the length of the vector
float srgs_utility_vec_magnitude(const srgs_vector3_t * a);

// Sets srcdest to a normalized version of itself.
void srgs_utility_vec_normalize(srgs_vector3_t * srcdest);
















/* Matrix utilities */






/* Standard operations */


// Sets a matrix dest to the values of matrix src
void srgs_utility_matrix_set(srgs_matrix_t * dest, srgs_matrix_t * src);

// Sets a matrix dest to be the matrix formed by all the elements given
void srgs_utility_matrix_set_float(srgs_matrix_t * dest,
	float x0, float y0, float z0, float w0,
	float x1, float y1, float z1, float w1,
	float x2, float y2, float z2, float w2,
	float x3, float y3, float z3, float w3
);

//Sets the matrix to the identity matrix
void srgs_utility_matrix_set_identity(srgs_matrix_t * dest);

// Sets result to  A * B
srgs_matrix_t srgs_utility_matrix_multiply_(srgs_matrix_t * a, srgs_matrix_t * b);

// Sets result as the result of A * B
void srgs_utility_matrix_multiply(srgs_matrix_t * result, const srgs_matrix_t * a, const srgs_matrix_t * b);

// Sets result to A * B
void srgs_utility_matrix_vector_multiply(srgs_vector4_t * result, const srgs_matrix_t * a, const srgs_vector4_t * b);


// Print the matrix
void srgs_utility_matrix_print(srgs_matrix_t * mat);

// Returns a read-only reference to an identity matrix;
srgs_matrix_t * srgs_utility_matrix_get_identity();

// Converts the matrix into its inverse (A transform matrix will always have an inverse.)
void srgs_utility_matrix_invert(srgs_matrix_t *);







/* Transform operations */

// Multiplies the matrix to express a rotational transform about the axis defined by a vector x, y, z.
void srgs_utility_matrix_rotate(srgs_matrix_t *, float theta, float xAxis, float yAxis, float zAxis);

// Multiplies the matrix to express a translation by x, y, and z
void srgs_utility_matrix_translate(srgs_matrix_t *, float x, float y, float z);

// Multiplies the matrix to express a scaling by x, y and z amounts
void srgs_utility_matrix_scale(srgs_matrix_t *, float x, float y, float z);

// Sets the matrix to express the inverse major order of its current form.
// The Matrix set of functions deal with matrices that are expressed in column-major format
void srgs_utility_matrix_reverse_majority(srgs_matrix_t *);

// Convenvience function for matrix * c assuming matrix is a transform matrix 
// and c is a positional vector. Internally, c is stepped up to a 4-component vector with 1.0 as its w 
// to properly calculation the output vector.
srgs_vector3_t srgs_utility_matrix_transform(const srgs_matrix_t * matrix, const srgs_vector3_t * c);

srgs_vector3_t srgs_utility_matrix_get_xyz_rotation(const srgs_matrix_t * matrix);




/* Projection operations */

// Multiplies the matrix by an orthographic projection matrix
void srgs_utility_matrix_projection_orthographic(srgs_matrix_t *,
	float left, float right,
	float bottom, float top,
	float near, float far
);


// Multiplies the matrix by a perspective matrix
void srgs_utility_matrix_projection_perspective(srgs_matrix_t *,
    float fieldOfViewThetaY,
    float aspectRatioXtoY,
    float zNear, //< MUST be positive
    float zFar   //< MUST be positive
);


void srgs_utility_matrix_look_at(srgs_matrix_t *,
    const srgs_vector3_t * eyePosition,
    const srgs_vector3_t * targetPosition,
    const srgs_vector3_t * upVector
);











#endif


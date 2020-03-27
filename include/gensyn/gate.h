#ifndef H_GENSYN_GATE__INCLUDED
#define H_GENSYN_GATE__INCLUDED

#include <gensyn/sample.h>
#include <gensyn/string.h>
#include <gensyn/array.h>
typedef struct gensyn_t gensyn_t;
/*
    GenSyn: Gate 
    
    Gates are the literal building blocks for
    the synthesizer. Each can transfer and/or receive data.
    
    Each gate has a number of "INs" and "OUTs" can be connected 
    based on the type of gate which grants much of its use. All 
    INs and OUTs are simply other gates.
    
    
    
    


*/

typedef struct gensyn_gate_t gensyn_gate_t;



// Called when the gate is created. This 
// is done on the same thread as the update and remove functions.
typedef void (*gensyn_gate__create_fn)(gensyn_gate_t *);

// Called when the gate is updated. This 
// is done on the same thread as the create and remove functions.
// Update happens before delivering data to the out gates.
typedef void (*gensyn_gate__update_fn)(
    gensyn_gate_t *, 
    gensyn_sample_t *, 
    uint32_t sampleCount,
    float sampleRate
);

// Called when the gate is updated. This 
// is done on the same thread as the create and remove functions.
typedef void (*gensyn_gate__remove_fn)(gensyn_gate_t *);





typedef enum {
    // The gate only provides input but does not consume any.
    GENSYN_GATE__TYPE__INPUT,

    // The gate only consumes input from other gates.
    GENSYN_GATE__TYPE__OUTPUT,
    
    // The gate consumes input and provides output.
    GENSYN_GATE__TYPE__TRANSFORM,
    
    // The gate has no type. This is reserved for gates with no OUTs or INs.
    GENSYN_GATE__TYPE__NULL,

} gensyn_gate__type_e;






typedef enum {
    GENSYN_GATE__PROPERTY__END,
    GENSYN_GATE__PROPERTY__IN,
    GENSYN_GATE__PROPERTY__OUT,
    GENSYN_GATE__PROPERTY__PARAM,

} gensyn_gate__property_e;

// Registers a new type of gate that can be instantiated by name 
// using gensyn_gate_create.
// name:        the name of the gate.
// onCreate:    The creation function for this gate.
// onUpdate:    The update function for this gate.
// onRemove:    The remove function for this gate.
//
// The remaining parameters are for defining properties of
// the gate.
//
// THis includes properties like INs and OUTs that 
// the gate will recognize and look for as well as named parameters. 
// In most cases, a gate will not function unless all INs are satisfied.
//
// The first string denotes what type of parameter it is
// Is is allowed to by the following:
//
//  GENSYN_GATE__PROPERTY_IN        Denotes the next string to be the name of an IN.
//  GENSYN_GATE__PROPERTY_OUT       Denotes the next string to be the name of an OUT
//  GENSYN_GATE__PROPERTY_PARAM     Denotes the next string to be the name of an parameter. Then it shall be followed by a double as a default value
//
//
// If the registration is successful, 1 is returned. Otherwise, 0 is returned 
// and the gate is not registered. 
int gensyn_gate_register(
    
    const gensyn_string_t *     name,

    gensyn_gate__create_fn      onCreate,
    gensyn_gate__update_fn      onUpdate,
    gensyn_gate__remove_fn      onRemove,
    
    ...
);



// Creates a new gate.
gensyn_gate_t *  gensyn_gate_create(gensyn_t *, const gensyn_string_t *);



// Destroys a gate. 
void gensyn_gate_destroy(gensyn_gate_t *);





// Runs the program consisteing of the entire gate circuit
// connected to this gate. 
void gensyn_gate_run(
    // The gate acting as output. 
    gensyn_gate_t *, 

    // The buffer to be written to.
    gensyn_sample_t *, 
    
    // the number of samples for this buffer
    uint32_t sampleCount,

    // the sample rate of the device to be using the 
    // buffer, in Hz.
    float sampleRate
);



// Sets the IN gate for the name.
void gensyn_gate_set_in(
    gensyn_gate_t *, 
    const gensyn_string_t * name, 
    gensyn_gate_t *
);


// Sets the OUT gate for the name.
void gensyn_gate_set_out(
    gensyn_gate_t *, 
    const gensyn_string_t * name, 
    gensyn_gate_t *
);





// Returns the value of a parameter
float gensyn_gate_get_parameter(const gensyn_gate_t *, const gensyn_string_t *);

// Sets the value of a parameter
void gensyn_gate_set_parameter(gensyn_gate_t *, const gensyn_string_t *, float);



// Gets an IN gate for the given registered IN.
// If none exists, NULL is returned.
gensyn_gate_t * gensyn_gate_get_in(const gensyn_gate_t *, const gensyn_string_t *);

// Gets an OUT gate for the given registered OUT.
// If none exists, NULL is returned.
gensyn_gate_t * gensyn_gate_get_out(const gensyn_gate_t *, const gensyn_string_t *);



const gensyn_array_t * gensyn_gate_get_in_names(const gensyn_gate_t *);

const gensyn_array_t * gensyn_gate_get_out_names(const gensyn_gate_t *);



// Returns what type of gate this is.
// This is based on the ins and outs specified during registration.
gensyn_gate__type_e gensyn_gate_get_type(const gensyn_gate_t *);



#endif

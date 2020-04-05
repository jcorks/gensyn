#include <gensyn/gate.h>
#include <gensyn/table.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
// In the interest of speed and simplicity, we'll
// ise a fixed maximum of INs/OUTs
#define MAX_CX 8
#define MAX_PARAM 32



// the base design of the gate interface allows for alterations to be made to the 
// gate mostly safely in parallel. No allocations are necessary to modification of the 
// gate nor in querying for the gate.
struct gensyn_gate_t {
    gensyn_t * context;
    gensyn_string_t * type;
    gensyn_gate__create_fn onCreate;
    gensyn_gate__update_fn onUpdate;
    gensyn_gate__remove_fn onRemove;

    int nins;
    int nouts;    
    int nparams;
    int texture;
    int x;
    int y;
    int isActive;
    float params[MAX_PARAM];

    gensyn_gate_t * inrefs [MAX_CX];
    gensyn_gate_t * outrefs[MAX_CX];

    
    gensyn_array_t * innamesArr;
    gensyn_array_t * paramnamesArr;

    gensyn_string_t * desc;
    void * data;

    gensyn_sample_t * sampleBuffer;
    uint32_t sampleBufferSize;
    uint32_t updateID;
    uint64_t sampleTick;
};





static gensyn_table_t * prefabs = NULL;

// Clones a prefab gate to make a new real gate.
static gensyn_gate_t * gensyn_gate_clone(const gensyn_gate_t *);


int gensyn_gate_register(
    
    const gensyn_string_t *     name,
    const gensyn_string_t *     desc,
    int                         texID,
    gensyn_gate__create_fn      onCreate,
    gensyn_gate__update_fn      onUpdate,
    gensyn_gate__remove_fn      onRemove,
    
    ...
) {
    if (!prefabs) {
        prefabs = gensyn_table_create_hash_gensyn_string();
    } else {

        // already registered.
        if (gensyn_table_entry_exists(prefabs, name)) {
            return 0;
        }
    }

    va_list args;
    va_start(args, onRemove);

    gensyn_gate_t * g = calloc(1, sizeof(gensyn_gate_t));

    g->onCreate = onCreate;
    g->onUpdate = onUpdate;
    g->onRemove = onRemove;
    g->desc = gensyn_string_clone(desc);
    g->texture = texID;
    g->type = gensyn_string_clone(name);
    g->innamesArr = gensyn_array_create(sizeof(gensyn_string_t*));
    g->paramnamesArr = gensyn_array_create(sizeof(gensyn_string_t*));
    
    
    int i;
    gensyn_string_t * entry;
    float dfparam;
L_START:
    switch(va_arg(args, gensyn_gate__property_e)) {
      case GENSYN_GATE__PROPERTY__CONNECTION:
        entry = va_arg(args, gensyn_string_t*);
        
        // max reached. error in registration
        if (g->nins >= MAX_CX) {
            gensyn_gate_destroy(g);
            return 0;                            
        }

        // already exists with this name. Error in registration
        for(i = 0; i < g->nins; ++i) {
            if (gensyn_string_test_eq(gensyn_array_at(g->innamesArr, gensyn_string_t *, i), entry)) {
                gensyn_gate_destroy(g);
                return 0;                                            
            }
        }            
        entry = gensyn_string_clone(entry);
        gensyn_array_push(g->innamesArr, entry);
        g->nins++;
        break;
            

      case GENSYN_GATE__PROPERTY__PARAM:
        entry = va_arg(args, gensyn_string_t*);
        dfparam = va_arg(args, double);        
        // max reached. error in registration
        if (g->nparams >= MAX_PARAM) {
            gensyn_gate_destroy(g);
            return 0;                            
        }

        // already exists with this name. Error in registration
        for(i = 0; i < g->nouts; ++i) {
            if (gensyn_string_test_eq(gensyn_array_at(g->paramnamesArr, gensyn_string_t *, i), entry)) {
                gensyn_gate_destroy(g);
                return 0;                                            
            }
        }            
        entry = gensyn_string_clone(entry);
        gensyn_array_push(g->paramnamesArr, entry);
        g->params[g->nparams++] = dfparam;      
        break;
      
      case GENSYN_GATE__PROPERTY__END:
        goto L_END;
    }    
    goto L_START;

L_END:
    gensyn_table_insert(prefabs, name, g);    
    va_end(args);
    return 1;
}


uint64_t gensyn_gate_get_sample_tick(const gensyn_gate_t * g) {
    return g->sampleTick;
}




gensyn_gate_t * gensyn_gate_create(gensyn_t * ctx, const gensyn_string_t * str) {
    gensyn_gate_t * prefab = gensyn_table_find(prefabs, str);
    if (!prefab) return NULL;

    gensyn_gate_t * out = gensyn_gate_clone(prefab);
    out->context = ctx;
    out->data = out->onCreate(out);
    return out;
}


void gensyn_gate_destroy(gensyn_gate_t * g) {
    int i, n;
    g->onRemove(g, g->data);

    for(i = 0; i < g->nins; ++i) {
        if (g->inrefs[i]) {
            for(n = 0; n < g->inrefs[i]->nouts; ++n) {
                if (g->inrefs[i]->outrefs[i] == g) {
                    g->inrefs[i]->outrefs[i] = NULL;
                    break;
                }
            }
        }
    }
    for(i = 0; i < g->nouts; ++i) {
        if (g->outrefs[i]) {
            for(n = 0; n < g->outrefs[i]->nouts; ++n) {
                if (g->outrefs[i]->inrefs[i] == g) {
                    g->outrefs[i]->inrefs[i] = NULL;
                    break;
                }
            }
        }
    }
    free(g);
}


static uint32_t updatePool = 0xff;

void gensyn_gate_run__internal(
    gensyn_gate_t * g, 
    uint32_t sampleCount,
    float sampleRate,
    uint32_t updateID
) {
    int i;
    gensyn_sample_t * inBuffers[g->nins];


    // circular dependency! abort
    if (g->updateID == updateID) {
        return;
    }
    g->updateID = updateID;

    // make sure internal buffer can handle it.
    if (g->sampleBufferSize <= sampleCount) {
        free(g->sampleBuffer);
        g->sampleBuffer = calloc(sampleCount, sizeof(gensyn_sample_t));
        g->sampleBufferSize = sampleCount;
    }  


    // always make sure dependencies are satisfied first.
    for(i = 0; i < g->nins; ++i) {
        if (g->inrefs[i]) {
            gensyn_gate_run__internal(
                g->inrefs[i],
                sampleCount,
                sampleRate,
                updateID
            );
            inBuffers[i] = g->inrefs[i]->sampleBuffer;
        } else {
            inBuffers[i] = NULL;
        }
    }

    // update local buffer
    g->onUpdate(
        g,
        g->nins,
        inBuffers,
        g->sampleBuffer,
        sampleCount,
        sampleRate,
        g->data
    );
    g->isActive = 1;
    g->sampleTick += sampleCount;
}


void gensyn_gate_run(
    gensyn_gate_t * g, 
    gensyn_sample_t * samplesOut, 
    uint32_t sampleCount,
    float sampleRate
) {
    gensyn_gate_run__internal(    
        g,
        sampleCount,
        sampleRate,
        ++updatePool
    );

    // write the final results
    memcpy(samplesOut, g->sampleBuffer, sampleCount*sizeof(gensyn_sample_t));
}

// Returns whether the gate was used last output cycle
int gensyn_gate_get_is_active(const gensyn_gate_t * g) {
    return g->isActive; 
}

// Resets the active flag for the gate. Normally, this is 
// run and controlled for you.
void gensyn_gate_reset_is_active(gensyn_gate_t * g) {
    g->isActive = 0;
}



// Sets the IN gate for the name.
void gensyn_gate_connect(
    gensyn_gate_t * from, 
    const gensyn_string_t * name, 
    gensyn_gate_t * to
) {
    int i;
    // max connections reached. 
    if (from && from->nouts >= MAX_CX) {
        return;
    }

    for(i = 0; i < to->nins; ++i) {
        if (gensyn_string_test_eq(gensyn_array_at(to->innamesArr, gensyn_string_t *, i), name)) {
            
            if (to->inrefs[i] != NULL) { // remove old ref from tree
                gensyn_gate_t * oldRef = to->inrefs[i];
                int n;
                for(n = 0; n < oldRef->nouts; ++n) {
                    if (oldRef->outrefs[n] == to) {
                        oldRef->outrefs[n] = NULL;
                        oldRef->nouts--;

                        // fill gap
                        for(; n < oldRef->nouts; ++n) {
                            oldRef->outrefs[n] = oldRef->outrefs[n+1]; 
                        }
                        break;
                    }
                }
            }
            if (from) {
                from->outrefs[from->nouts++] = to;
            }
            to->inrefs[i] = from;
            return;
        }
    }
}


void gensyn_gate_disconnect(
    gensyn_gate_t * from, 
    const gensyn_string_t * name, 
    gensyn_gate_t * to
) {
    gensyn_gate_connect(NULL, name, to);
}






// Returns the value of a parameter
float gensyn_gate_get_parameter(const gensyn_gate_t * g, const gensyn_string_t * name) {
    int i;
    for(i = 0; i < g->nparams; ++i) {
        if (gensyn_string_test_eq(gensyn_array_at(g->paramnamesArr, gensyn_string_t *, i), name)) {
            return g->params[i];
        }
    }
    return 0.f;
}

// Sets the value of a parameter
void gensyn_gate_set_parameter(gensyn_gate_t * g, const gensyn_string_t * name, float data) {
    int i;
    for(i = 0; i < g->nparams; ++i) {
        if (gensyn_string_test_eq(gensyn_array_at(g->paramnamesArr, gensyn_string_t *, i), name)) {
            g->params[i] = data;
        }
    }
    
}




// Gets an IN gate for the given registered IN.
// If none exists, NULL is returned.
gensyn_gate_t * gensyn_gate_get_in_connection(const gensyn_gate_t * g, const gensyn_string_t * name) {
    int i;
    for(i = 0; i < g->nins; ++i) {
        if (gensyn_string_test_eq(gensyn_array_at(g->innamesArr, gensyn_string_t *, i), name)) {
            return g->inrefs[i];
        }
    }
    return NULL;
}

// Gets an OUT gate for the given registered OUT.
// If none exists, NULL is returned.
gensyn_gate_t * gensyn_gate_get_out_connection(const gensyn_gate_t * g, int index) {
    return g->outrefs[index];
}


const gensyn_array_t * gensyn_gate_get_in_names(const gensyn_gate_t * g) {
    return g->innamesArr;
}

const gensyn_array_t * gensyn_gate_get_param_names(const gensyn_gate_t * g) {
    return g->paramnamesArr;
}


int gensyn_gate_get_out_connection_count(const gensyn_gate_t * g) {
    return g->nouts;
}


gensyn_gate__type_e gensyn_gate_get_type(const gensyn_gate_t * g) {
    if (g->nins && g->nouts) 
        return GENSYN_GATE__TYPE__TRANSFORM;
    
    if (g->nins && !g->nouts)
        return GENSYN_GATE__TYPE__OUTPUT;
    
    if (g->nouts && !g->nins) 
        return GENSYN_GATE__TYPE__INPUT;
    
    return GENSYN_GATE__TYPE__NULL;
}


const gensyn_string_t * gensyn_gate_get_description(const gensyn_gate_t * g) {
    return g->desc;
}

const gensyn_string_t * gensyn_gate_get_class(const gensyn_gate_t * g) {
    return g->type;
}


int gensyn_gate_get_x(const gensyn_gate_t * g) {
    return g->x;
}

int gensyn_gate_get_y(const gensyn_gate_t * g) {
    return g->y;
}

void gensyn_gate_set_x(gensyn_gate_t * g, int x) {
    g->x = x;
}
void gensyn_gate_set_y(gensyn_gate_t * g, int y) {
    g->y = y;
}


//////// statics 


gensyn_gate_t * gensyn_gate_clone(const gensyn_gate_t * src) {
    gensyn_gate_t * g = malloc(sizeof(gensyn_gate_t));
    // since the arrays for names are readonly and all refs are started at 0 anyway,
    // it should be, for once, safe to do a shallow copy.
    *g = *src;

    return g;
}

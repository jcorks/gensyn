#include <gensyn/gate.h>
#include <gensyn/table.h>
#include <stdarg.h>
#include <stdlib.h>
// In the interest of speed and simplicity, we'll
// ise a fixed maximum of INs/OUTs
#define MAX_CX 8
#define MAX_PARAM 32



// the base design of the gate interface allows for alterations to be made to the 
// gate mostly safely in parallel. No allocations are necessary to modification of the 
// gate nor in querying for the gate.
struct gensyn_gate_t {
    gensyn_t * context;
    
    gensyn_gate__create_fn onCreate;
    gensyn_gate__update_fn onUpdate;
    gensyn_gate__remove_fn onRemove;

    int nins;
    int nouts;    
    int nparams;
    float params[MAX_PARAM];

    gensyn_gate_t * inrefs [MAX_CX];
    gensyn_gate_t * outrefs[MAX_CX];

    
    gensyn_array_t * innamesArr;
    gensyn_array_t * outnamesArr;
    gensyn_array_t * paramnamesArr;
};





static gensyn_table_t * prefabs = NULL;

// Clones a prefab gate to make a new real gate.
static gensyn_gate_t * gensyn_gate_clone(const gensyn_gate_t *);


int gensyn_gate_register(
    
    const gensyn_string_t *     name,
    
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
    



    int i;
    gensyn_string_t * entry;
    float dfparam;
L_START:
    switch(va_arg(args, gensyn_gate__property_e)) {
      case GENSYN_GATE__PROPERTY__IN:
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
            
      case GENSYN_GATE__PROPERTY__OUT:
        entry = va_arg(args, gensyn_string_t*);
        
        // max reached. error in registration
        if (g->nouts >= MAX_CX) {
            gensyn_gate_destroy(g);
            return 0;                            
        }

        // already exists with this name. Error in registration
        for(i = 0; i < g->nouts; ++i) {
            if (gensyn_string_test_eq(gensyn_array_at(g->outnamesArr, gensyn_string_t *, i), entry)) {
                gensyn_gate_destroy(g);
                return 0;                                            
            }
        }            
        entry = gensyn_string_clone(entry);
        gensyn_array_push(g->outnamesArr, entry);
        g->nouts++;
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



gensyn_gate_t * gensyn_gate_create(gensyn_t * ctx, const gensyn_string_t * str) {
    gensyn_gate_t * prefab = gensyn_table_find(prefabs, str);
    if (!prefab) return NULL;

    gensyn_gate_t * out = gensyn_gate_clone(prefab);
    out->context = ctx;
    return out;
}


void gensyn_gate_destroy(gensyn_gate_t * g) {
    int i, n;


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




// Sets the IN gate for the name.
void gensyn_gate_set_in(
    gensyn_gate_t * g, 
    const gensyn_string_t * name, 
    gensyn_gate_t * next
) {
    int i;
    for(i = 0; i < g->nins; ++i) {
        if (gensyn_string_test_eq(gensyn_array_at(g->innamesArr, gensyn_string_t *, i), name)) {
            if (g->inrefs[i] != NULL) { // remove old ref from tree
                gensyn_gate_t * oldRef = g->inrefs[i];
                int n;
                for(n = 0; n < oldRef->nouts; ++n) {
                    if (oldRef->outrefs[n] == g) {
                        oldRef->outrefs[n] = NULL;
                        break;
                    }
                }
            }
            g->inrefs[i] = next;
            return;
        }
    }
}


// Sets the OUT gate for the name.
void gensyn_gate_set_out(
    gensyn_gate_t * g, 
    const gensyn_string_t * name, 
    gensyn_gate_t * next
) {
    int i;
    for(i = 0; i < g->nouts; ++i) {
        if (gensyn_string_test_eq(gensyn_array_at(g->outnamesArr, gensyn_string_t *, i), name)) {
            if (g->outrefs[i] != NULL) { // remove old ref from tree
                gensyn_gate_t * oldRef = g->outrefs[i];
                int n;
                for(n = 0; n < oldRef->nins; ++n) {
                    if (oldRef->inrefs[n] == g) {
                        oldRef->inrefs[n] = NULL;
                        break;
                    }
                }
            }
            g->outrefs[i] = next;
            return;
        }
    }
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
gensyn_gate_t * gensyn_gate_get_in(const gensyn_gate_t * g, const gensyn_string_t * name) {
    int i;
    for(i = 0; i < g->nparams; ++i) {
        if (gensyn_string_test_eq(gensyn_array_at(g->innamesArr, gensyn_string_t *, i), name)) {
            return g->inrefs[i];
        }
    }
    return NULL;
}

// Gets an OUT gate for the given registered OUT.
// If none exists, NULL is returned.
gensyn_gate_t * gensyn_gate_get_out(const gensyn_gate_t * g, const gensyn_string_t * name) {
    int i;
    for(i = 0; i < g->nparams; ++i) {
        if (gensyn_string_test_eq(gensyn_array_at(g->outnamesArr, gensyn_string_t *, i), name)) {
            return g->outrefs[i];
        }
    }
    return NULL;    
}


const gensyn_array_t * gensyn_gate_get_in_names(const gensyn_gate_t * g) {
    return g->innamesArr;
}

const gensyn_array_t * gensyn_gate_get_out_names(const gensyn_gate_t * g) {
    return g->outnamesArr;
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



//////// statics 


gensyn_gate_t * gensyn_gate_clone(const gensyn_gate_t * src) {
    gensyn_gate_t * g = malloc(sizeof(gensyn_gate_t));
    // since the arrays for names are readonly and all refs are started at 0 anyway,
    // it should be, for once, save to do a shallow copy.
    *g = *src;

    return g;
}

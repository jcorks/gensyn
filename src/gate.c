#include <gensyn/gate.h>
#include <gensyn/table.h>


// In the interest of speed and simplicity, we'll
// ise a fixed maximum of INs/OUTs
#define MAX_CX 8
#define MAX_PARAM 32

struct gensyn_gate_t {
    gensyn_t * context;
    
    gensyn_gate__create_fn onCreate;
    gensyn_gate__update_fn onUpdate;
    gensyn_gate__remove_fn onRemove;

    int nins;
    int nouts;    
    int nparams;
    gensyn_string_t * inslots [MAX_CX];
    gensyn_string_t * outslots[MAX_CX];

    gensyn_gate_t * inrefs [MAX_CX];
    gensyn_gate_t * outrefs[MAX_CX];

    gensyn_string_t * paramslots[MAX_PARAM];    
    float params[MAX_PARAM];
};





static gensyn_table_t * prefabs = NULL;

// Clones a prefab gate to make a new real gate.
static gensyn_t * gensyn_gate_clone(const gensyn_gate_t *);


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

    va_list args = va_start(onRemove);

    gensyn_gate_t * g = calloc(1, sizeof(gensyn_gate_t));

    g->onCreate = onCreate;
    g->onUpdate = onUpdate;
    g->onRemove = onRemove;
    



    int i;
    gensyn_string_t * entry;
L_START:
    switch(va_arg(args, gensyn_gate__property_e)) {
      case GENSYN_GATE__PROPERTY_IN:
        entry = va_arg(args, gensyn_string_t);
        
        // max reached. error in registration
        if (g->nins >= MAX_CX) {
            gensyn_gate_destroy(g);
            return 0;                            
        }

        // already exists with this name. Error in registration
        for(i = 0; i < g->nins; ++i) {
            if (gensyn_string_test_eq(g->inslots[i]), entry)) {
                gensyn_gate_destroy(g);
                return 0;                                            
            }
        }            
        g->inslots[g->nins++] = gensyn_string_clone(entry);
        break;
            
      case GENSYN_GATE__PROPERTY_OUT:
        entry = va_arg(args, gensyn_string_t);
        
        // max reached. error in registration
        if (g->nouts >= MAX_CX) {
            gensyn_gate_destroy(g);
            return 0;                            
        }

        // already exists with this name. Error in registration
        for(i = 0; i < g->nouts; ++i) {
            if (gensyn_string_test_eq(g->outslots[i]), entry)) {
                gensyn_gate_destroy(g);
                return 0;                                            
            }
        }            
        g->outslots[g->nins++] = gensyn_string_clone(entry);
        break;

      case GENSYN_GATE__PROPERTY_PARAM:
        entry = va_arg(args, gensyn_string_t);
        dfparam = va_arg(args, float);        
        // max reached. error in registration
        if (g->nparams >= MAX_PARAM) {
            gensyn_gate_destroy(g);
            return 0;                            
        }

        // already exists with this name. Error in registration
        for(i = 0; i < g->nparams; ++i) {
            if (gensyn_string_test_eq(g->paramslots[i]), entry)) {
                gensyn_gate_destroy(g);
                return 0;                                            
            }
        }      
        g->params[g->nins] = dfparam;      
        g->paramslots[g->nins++] = gensyn_string_clone(entry);
        break;
      
      case GENSYN_GATE__PROPERTY_END:
        goto L_END;
    }    
    goto L_START;

L_END:
    gensyn_table_insert(prefabs, g);    
    va_end(args);
    return 1;
}



gensyn_gate_t * gensyn_gate_create(gensyn_t * ctx, const gensyn_string_t * str) {
    gensyn_gate_t * prefab = gensyn_table_find(prefabs, str);
    if (!prefab) return;

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
        gensyn_string_destroy(g->inslots[i]);
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

        gensyn_string_destroy(g->outslots[i]);
    }
    free(g);
}


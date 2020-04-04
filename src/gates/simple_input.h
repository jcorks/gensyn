


static void * simple_input__on_create(gensyn_gate_t * g) {
    return NULL;
}

static int simple_input__on_update(
    gensyn_gate_t *     gate, 
    int                 nIn,
    gensyn_sample_t **  inSampleBuffers, 
    gensyn_sample_t *   buffer,
    uint32_t            sampleCount,
    float               sampleRate,
    void *              userData
) {
    float val = gensyn_gate_get_parameter(gate, GENSYN_STR_CAST("value"));
    uint32_t i;
    for(i = 0; i < sampleCount; ++i) {
        buffer[i] = val;
    }
    return 1;
}

static void simple_input__on_remove(gensyn_gate_t * g, void * data) {
    
}


void gensyn_gate_add__simple_input() {
    gensyn_gate_register(
        GENSYN_STR_CAST("Simple_Input"),
        GENSYN_STR_CAST("Provides a simple, static value."),

        1,
        simple_input__on_create,
        simple_input__on_update,
        simple_input__on_remove,
        
        
        GENSYN_GATE__PROPERTY__PARAM, GENSYN_STR_CAST("value"), .5,
        GENSYN_GATE__PROPERTY__END
    );
    
    
}

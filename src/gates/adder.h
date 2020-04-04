


static void * adder__on_create(gensyn_gate_t * g) {
    return NULL;
}

static int adder__on_update(
    gensyn_gate_t *     gate, 
    int                 nIn,
    gensyn_sample_t **  inSampleBuffers, 
    gensyn_sample_t *   buffer,
    uint32_t            sampleCount,
    float               sampleRate,
    void *              userData
) {

    uint32_t i, n;
    uint16_t sampleTick = gensyn_gate_get_sample_tick(gate);
    gensyn_sample_t * ins[nIn];
    int inReal = 0;
    for(i = 0; i < nIn; ++i) {
        if (inSampleBuffers[i])
            ins[inReal++] = inSampleBuffers[i];
    }
    
    float highest = 0;
    for(i = 0; i < sampleCount; ++i) {
        buffer[i] = 0;
        for(n = 0; n < inReal; ++n) {
            buffer[i] += ins[n][i];
        }
        if (fabs(buffer[i]) > highest)
            highest = fabs(buffer[i]);
    }
    
    // normalize
    if (highest > 0 && (gensyn_gate_get_parameter(gate, GENSYN_STR_CAST("normalize")) > .5)) {
        for(i = 0; i < sampleCount; ++i) {
            buffer[i] /= highest;            
        }
    }
    return 1;
}

static void adder__on_remove(gensyn_gate_t * g, void * data) {
    
}


void gensyn_gate_add__adder() {
    gensyn_gate_register(
        GENSYN_STR_CAST("Adder"),
        GENSYN_STR_CAST("Takes multiple gates and adds their output together. The output is normalized."),

        1,
        adder__on_create,
        adder__on_update,
        adder__on_remove,
        
        
        GENSYN_GATE__PROPERTY__CONNECTION,  GENSYN_STR_CAST("input0"),
        GENSYN_GATE__PROPERTY__CONNECTION,  GENSYN_STR_CAST("input1"),
        GENSYN_GATE__PROPERTY__CONNECTION,  GENSYN_STR_CAST("input2"),
        GENSYN_GATE__PROPERTY__CONNECTION,  GENSYN_STR_CAST("input3"),
        GENSYN_GATE__PROPERTY__CONNECTION,  GENSYN_STR_CAST("input4"),
        GENSYN_GATE__PROPERTY__CONNECTION,  GENSYN_STR_CAST("input5"),
        GENSYN_GATE__PROPERTY__CONNECTION,  GENSYN_STR_CAST("input6"),
        GENSYN_GATE__PROPERTY__CONNECTION,  GENSYN_STR_CAST("input7"),

        // whether to normalize the gate
        GENSYN_GATE__PROPERTY__PARAM,   GENSYN_STR_CAST("normalize"), 0.0,
                         
        GENSYN_GATE__PROPERTY__END
    );
    
    
}

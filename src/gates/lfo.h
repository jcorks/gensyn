


static void * lfo__on_create(gensyn_gate_t * g) {
    return NULL;
}

static int lfo__on_update(
    gensyn_gate_t *     gate, 
    int                 nIn,
    gensyn_sample_t **  inSampleBuffers, 
    gensyn_sample_t *   buffer,
    uint32_t            sampleCount,
    float               sampleRate,
    void *              userData
) {
    float hz  = gensyn_gate_get_parameter(gate, GENSYN_STR_CAST("hz"));
    float max = gensyn_gate_get_parameter(gate, GENSYN_STR_CAST("max"));

    if (max < 0) max = 0;
    if (max > 1) max = 1;
    
    uint32_t i;
    uint16_t sampleTick = gensyn_gate_get_sample_tick(gate);
    
    for(i = 0; i < sampleCount; ++i) {
        buffer[i] = (sin(M_PI * 2 * ((i+sampleTick) / sampleRate)*hz))*max;
    }
    return 1;
}

static void lfo__on_remove(gensyn_gate_t * g, void * data) {
    
}


void gensyn_gate_add__lfo() {
    gensyn_gate_register(
        GENSYN_STR_CAST("Simple_LFO"),
        GENSYN_STR_CAST("Provides simple, low-frequency oscillation as input"),

        1,
        lfo__on_create,
        lfo__on_update,
        lfo__on_remove,
        NULL,

        
        GENSYN_GATE__PROPERTY__PARAM, GENSYN_STR_CAST("hz"),   .5,
        GENSYN_GATE__PROPERTY__PARAM, GENSYN_STR_CAST("max"), 1.0,

        GENSYN_GATE__PROPERTY__END
    );
    
    
}

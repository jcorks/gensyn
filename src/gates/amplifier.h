


static void * amplifier__on_create(gensyn_gate_t * g) {
    return NULL;
}

static int amplifier__on_update(
    gensyn_gate_t *     gate, 
    int                 nIn,
    gensyn_sample_t **  inSampleBuffers, 
    gensyn_sample_t *   buffer,
    uint32_t            sampleCount,
    float               sampleRate,
    void *              userData
) {
    float volume  = gensyn_gate_get_parameter(gate, GENSYN_STR_CAST("volume"));
    if (!inSampleBuffers[0]) return 0;
    
    uint32_t i;
    
    for(i = 0; i < sampleCount; ++i) {
        buffer[i] = (inSampleBuffers[0][i] * volume);

        if (buffer[i] >  1.f) buffer[i] =  1.f;
        if (buffer[i] < -1.f) buffer[i] = -1.f;
    }
    return 1;
}

static void amplifier__on_remove(gensyn_gate_t * g, void * userData) {
    
}


void gensyn_gate_add__amplifier() {
    gensyn_gate_register(
        GENSYN_STR_CAST("Simple_Amplifier"),
        GENSYN_STR_CAST("Aplifies or lessens the incoming sorce by scaling it. Amplitudes are clipped."),

        1,
        amplifier__on_create,
        amplifier__on_update,
        amplifier__on_remove,
        
        
        GENSYN_GATE__PROPERTY__CONNECTION,  GENSYN_STR_CAST("input"),
        GENSYN_GATE__PROPERTY__PARAM,       GENSYN_STR_CAST("volume"),  1.0,

        GENSYN_GATE__PROPERTY__END
    );
    
    
}

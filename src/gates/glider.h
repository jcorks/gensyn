


typedef struct {
    float prevSample;
    
} glider__data_t;


static void * glider__on_create(gensyn_gate_t * g) {
    return calloc(1, sizeof(glider__data_t));
}

static int glider__on_update(
    gensyn_gate_t *     gate, 
    int                 nIn,
    gensyn_sample_t **  inSampleBuffers, 
    gensyn_sample_t *   buffer,
    uint32_t            sampleCount,
    float               sampleRate,
    void *              userData
) {
    if (!inSampleBuffers[0]) return 0;    
    glider__data_t * src = userData;
    
    
    
    float val = gensyn_gate_get_parameter(gate, GENSYN_STR_CAST("interp_amount"));
    if (val > .99999) val = .99999;
    if (val < .00001) val = .00001;
    
    
    uint32_t i;
    for(i = 0; i < sampleCount; ++i) {
        buffer[i] = inSampleBuffers[0][i]*val + src->prevSample*(1.0-val);
        src->prevSample = buffer[i];
    }
    return 1;
}

static void glider__on_remove(gensyn_gate_t * g, void * data) {
    
}


void gensyn_gate_add__glider() {
    gensyn_gate_register(
        GENSYN_STR_CAST("Glider"),
        GENSYN_STR_CAST("Will interpolate between successive sample values."),

        1,
        glider__on_create,
        glider__on_update,
        glider__on_remove,
        NULL,

        
        GENSYN_GATE__PROPERTY__CONNECTION, GENSYN_STR_CAST("input"),
        
        GENSYN_GATE__PROPERTY__PARAM, GENSYN_STR_CAST("interp_amount"), .1,
        GENSYN_GATE__PROPERTY__END
    );
    
    
}

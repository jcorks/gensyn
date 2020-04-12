


static void * gensyn_output__on_create(gensyn_gate_t * g) {
    return NULL;
}

static int gensyn_output__on_update(
    gensyn_gate_t *     gate, 
    int                 nIn,
    gensyn_sample_t **  inSampleBuffers, 
    gensyn_sample_t *   buffer,
    uint32_t            sampleCount,
    float               sampleRate,
    void *              userData
) {
    if (!inSampleBuffers[0]) {
        // missing input! nothing to write to device...
        printf("No input. Nothing to do!\n");
        return 0;
    }

    printf("output @ %d samples, %.2f Hz\n", sampleCount, sampleRate);
    memcpy(buffer, inSampleBuffers[0], sizeof(gensyn_sample_t)*sampleCount);
    return 1;
}

static void gensyn_output__on_remove(gensyn_gate_t * g, void * userData) {
    
}


void gensyn_gate_add__gensyn_output() {
    gensyn_gate_register(
        GENSYN_STR_CAST("GenSyn_Output"),
        GENSYN_STR_CAST("Acts as the symbolic receiver of the waveform. The recevied waveform is then passed to the device to be output as raw audio. As such, this is the endpoint for the synth."),

        1,
        gensyn_output__on_create,
        gensyn_output__on_update,
        gensyn_output__on_remove,
        NULL,

        
        GENSYN_GATE__PROPERTY__CONNECTION, GENSYN_STR_CAST("waveform"),
        GENSYN_GATE__PROPERTY__END
    );
    
    
}

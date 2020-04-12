



typedef struct {
    float lastFreq;
    float lastSample;
    uint64_t sampleTrack;    
} sine_wave__data_t;

static void * sine_wave__on_create(gensyn_gate_t * g) {
    return calloc(1, sizeof(sine_wave__data_t));
}

static int sine_wave__on_update(
    gensyn_gate_t *     gate, 
    int                 nIn,
    gensyn_sample_t **  inSampleBuffers, 
    gensyn_sample_t *   buffer,
    uint32_t            sampleCount,
    float               sampleRate,
    void *              dataSrc
) {

    gensyn_sample_t * pitch = inSampleBuffers[0];
    gensyn_sample_t * phase = inSampleBuffers[1];
    gensyn_sample_t * velocity = inSampleBuffers[2];
    
    uint32_t i;
    sine_wave__data_t * src = dataSrc;
    
    printf("@ %f\n", gensyn_pitch_sample_to_hz(pitch[0]));
    
    if (pitch && phase){
        /*
        for(i = 0; i < sampleCount; ++i) {
            buffer[i] = sin(
                2 * M_PI * (// rads -> 1.0 per cycle

                    phase[i] + // offset in seconds
                    
                    (gensyn_pitch_sample_to_hz(pitch[i])) * // 1.0 cycle -> number of cycles per second,
                    
                    ((i+sampleOffset) / sampleRate)  // progress of the cycle
                )
                
            )*.5 + .5;
        }*/
    } else if (pitch) {
        for(i = 0; i < sampleCount; ++i) {
            buffer[i] = sin( 
                2 * M_PI * (// rads -> 1.0 per cycle
                    
                    (src->lastFreq) * // 1.0 cycle -> number of cycles per second,
                    
                    ((src->sampleTrack) / sampleRate)  // progress of the cycle
                )
            );
            
            // we only want to allow frequency shifts when we are not midway through a cycle
            if (src->lastSample <= 0 && buffer[i] >= 0) {
                src->sampleTrack = 0;
                src->lastFreq = (gensyn_pitch_sample_to_hz(pitch[i]));
            }
            src->sampleTrack++;
            src->lastSample = buffer[i];
        }         
    } else 
        return 0;
    
    
    if (velocity) {
        for(i = 0; i < sampleCount; ++i) {
            buffer[i]*=velocity[i];
        }
    }
    return 1;
}

static void sine_wave__on_remove(gensyn_gate_t * g, void * data) {
    free(data);
}


void gensyn_gate_add__sine_wave() {
    gensyn_gate_register(
        GENSYN_STR_CAST("Sine_Wave"),
        GENSYN_STR_CAST("Outputs a simple sine wave"),

        1,
        sine_wave__on_create,
        sine_wave__on_update,
        sine_wave__on_remove,
        NULL,

        
        GENSYN_GATE__PROPERTY__CONNECTION, GENSYN_STR_CAST("pitch"),
        GENSYN_GATE__PROPERTY__CONNECTION, GENSYN_STR_CAST("velocity"),
        GENSYN_GATE__PROPERTY__CONNECTION, GENSYN_STR_CAST("phase"),


        GENSYN_GATE__PROPERTY__END
    );
    
    
}




static void sine_wave__on_create(gensyn_gate_t * g) {
    
}

static int sine_wave__on_update(
    gensyn_gate_t *     gate, 
    int                 nIn,
    gensyn_sample_t **  inSampleBuffers, 
    gensyn_sample_t *   buffer,
    uint32_t            sampleCount,
    float               sampleRate
) {

    gensyn_sample_t * pitch = inSampleBuffers[0];
    gensyn_sample_t * phase = inSampleBuffers[1];
    gensyn_sample_t * velocity = inSampleBuffers[2];
    
    uint32_t i;
    uint64_t sampleOffset = gensyn_gate_get_sample_tick(gate);
    
    printf("@ %f\n", gensyn_pitch_sample_to_hz(pitch[0]));
    
    if (pitch && phase){
        for(i = 0; i < sampleCount; ++i) {
            buffer[i] = sin(
                2 * M_PI * (// rads -> 1.0 per cycle

                    phase[i] + // offset in seconds
                    
                    (gensyn_pitch_sample_to_hz(pitch[i])) * // 1.0 cycle -> number of cycles per second,
                    
                    ((i+sampleOffset) / sampleRate)  // progress of the cycle
                )
                
            )*.5 + .5;
        }
    } else if (pitch) {
        for(i = 0; i < sampleCount; ++i) {
            buffer[i] = sin(
                2 * M_PI * (// rads -> 1.0 per cycle
                    
                    (gensyn_pitch_sample_to_hz(pitch[i])) * // 1.0 cycle -> number of cycles per second,
                    
                    ((i+sampleOffset) / sampleRate)  // progress of the cycle
                )
                
            )*.5 + .5;
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

static void sine_wave__on_remove(gensyn_gate_t * g) {
    
}


void gensyn_gate_add__sine_wave() {
    gensyn_gate_register(
        GENSYN_STR_CAST("Sine_Wave"),
        GENSYN_STR_CAST("Outputs a simple sine wave"),

        1,
        sine_wave__on_create,
        sine_wave__on_update,
        sine_wave__on_remove,
        
        
        GENSYN_GATE__PROPERTY__CONNECTION, GENSYN_STR_CAST("pitch"),
        GENSYN_GATE__PROPERTY__CONNECTION, GENSYN_STR_CAST("velocity"),
        GENSYN_GATE__PROPERTY__CONNECTION, GENSYN_STR_CAST("phase"),


        GENSYN_GATE__PROPERTY__END
    );
    
    
}

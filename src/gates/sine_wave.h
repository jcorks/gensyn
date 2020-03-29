


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
    
    uint32_t i;
    
    
    
    if (pitch && phase){
        for(i = 0; i < sampleCount; ++i) {
            buffer[i] = sin(
                2 * M_PI * (// rads -> 1.0 per cycle

                    phase[i] + // offset in seconds
                    
                    (gensyn_pitch_sample_to_hz(i)) * // 1.0 cycle -> number of cycles per second,
                    
                    (i / sampleRate)  // progress of the cycle
                )
                
            );
        }
    } else if (pitch) {
        for(i = 0; i < sampleCount; ++i) {
            buffer[i] = sin(
                2 * M_PI * (// rads -> 1.0 per cycle
                    
                    (gensyn_pitch_sample_to_hz(i)) * // 1.0 cycle -> number of cycles per second,
                    
                    (i / sampleRate)  // progress of the cycle
                )
                
            );
        }
        
        
    } else 
        return 0;
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
        GENSYN_GATE__PROPERTY__CONNECTION, GENSYN_STR_CAST("phase"),


        GENSYN_GATE__PROPERTY__END
    );
    
    
}

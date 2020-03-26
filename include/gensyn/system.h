#ifndef H_GENSYN_SYSTEM__INCLUDED
#define H_GENSYN_SYSTEM__INCLUDED



// Gets the input pointer 
void gensyn_system_get_pointer(
    gensyn_t *, 
    int * down0,
    int * down1,
    int * x,
    int * y
);

// Processes input from the user
void gensyn_system_update_input(gensyn_t *)



// Pushes the framebuffer to the screen.
void gensyn_system_update_screen(gensyn_t *);



// Sets up the audio stream. Once started, 
// the stream will continuously call the stream callback
// as the audio device needs.
void gensyn_system_setup_audio(
    gensyn_t *,
    void (*)(gensyn_t *, gensyn_sample_t * samples, uint32_t numSamples, float sampleRate)
);
















#endif

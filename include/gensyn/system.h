#ifndef H_GENSYN_SYSTEM__INCLUDED
#define H_GENSYN_SYSTEM__INCLUDED

#include <gensyn/string.h>
#include <gensyn/sample.h>
typedef struct srgs_t srgs_t;
typedef struct gensyn_t gensyn_t;




typedef struct gensyn_system_t gensyn_system_t;







gensyn_system_t * gensyn_system_create();




/////////////////
///////////////// INPUT 
/////////////////


typedef enum {
    GENSYN_SYSTEM__INPUT__CONFIRM = 0xff01,
    GENSYN_SYSTEM__INPUT__DENY,
    GENSYN_SYSTEM__INPUT__OPTIONS,
    GENSYN_SYSTEM__INPUT__LEFT,
    GENSYN_SYSTEM__INPUT__RIGHT,
    GENSYN_SYSTEM__INPUT__UP,
    GENSYN_SYSTEM__INPUT__DOWN,
} gensyn_system__input_e;


typedef struct {
    // device ID 
    int deviceID; 
    
    // input
    // If <= 0xff, it refers to a MIDI control code
    // Else, it will refer to a gensyn_system_input_e value 
    //
    // If this is a midicontrol code, the 
    // raw byte data can be found in midiData1
    // and midiData2
    //
    int input;
    
    // if < 0xff these values are raw midi data. If input > 0xff,
    // inputData1 is the current incoming updated value 
    // inputData2 is the previous data value.
    uint8_t inputData1;
    uint8_t inputData2;
    
} gensyn_system__input_event_t;


// Attempts to get device information
int gensyn_system_input_get_device_info(
    gensyn_system_t *,
    
    // the device ID. Always starts at 0 and goes 
    // to number fo devices. If unsuccessful, returns 0
    int deviceID,

    // String to populate with the name. May be empty.
    gensyn_string_t * name_out,

    // string to populate with description. May be empty.
    gensyn_string_t * desc_out
);


// Requeries what devices are available. The total devices 
// found is returned
int gensyn_system_input_query_devices(gensyn_system_t *);

// Poles and processes input from the user. This includes 
// midi events
void gensyn_system_input_update(gensyn_system_t *);


// Returns how many events are remaining after 
// calling this function
int gensyn_system_input_get_events(gensyn_system_t *,
    gensyn_system__input_event_t * events,
    uint32_t                       eventsMax,
    uint32_t *                     eventsReceived
);


// Sends an input event onto the queue.
// Useful for simulating events externally
void gensyn_system_input_send_event(gensyn_system_t *, const gensyn_system__input_event_t *);






/////////////////
///////////////// GRAPHICS
/////////////////



// returns the graphics context
srgs_t * gensyn_system_graphics_get(gensyn_system_t *);

// Pushes the framebuffer to the screen.
void gensyn_system_update_screen(gensyn_system_t *);






/////////////////
///////////////// AUDIO
/////////////////


// Sets up the audio stream. Once started, 
// the stream will continuously call the stream callback
// as the audio device needs.
void gensyn_system_setup_audio(
    gensyn_system_t *,
    void (*)(void * userData, gensyn_sample_t * samples, uint32_t numSamples, float sampleRate),
    void * userData
);


/////////////////
///////////////// UTILITY
/////////////////

void gensyn_system_usleep(uint32_t);

uint8_t gensyn_system_thread_create(gensyn_system_t *, void * (*)(void *), void *);

void gensyn_system_thread_cancel(gensyn_system_t *, uint8_t);











#endif

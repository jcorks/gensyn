#if ( __unix__ | __linux__)



#include <unistd.h>
#include <gensyn/system.h>
#include <gensyn/gensyn.h>
#include <gensyn/string.h>
#include <gensyn/array.h>
#include <stdlib.h>

#include <alsa/asoundlib.h>
#include <pthread.h>

// Runs the give program with the given arguments.
// standard out for that 
static gensyn_string_t * exec_capture_output(const gensyn_string_t * prog, const gensyn_string_t * args);



// Handles input-related functions. Implemented below.
typedef struct gensyn_linux_input_t gensyn_linux_input_t;

// Creates an input instance.
static gensyn_linux_input_t * gensyn_linux_input_create();



struct gensyn_system_t {
    gensyn_linux_input_t * input;    
};


gensyn_system_t * gensyn_system_create() {
    gensyn_system_t * out = malloc(sizeof(gensyn_system_t));
    out->input = gensyn_linux_input_create();
    return out;
}









////////////INPUT          ///////////////
////////////IMPLEMENTATION //////////////



// event queue
typedef struct ev_queue_t ev_queue_t;

// Creates a new event queue
static ev_queue_t * ev_queue_create();

// Destroys an event queue
static void ev_queue_destroy(ev_queue_t *);;

// Pops an event from the queue
// If none left, object is empty
static gensyn_system__input_event_t ev_queue_pop(ev_queue_t *);

// Returns whether the event queue is empty
static int ev_queue_empty(const ev_queue_t *);

// Pushes an evetn to the queue
static void ev_queue_push(ev_queue_t *, const gensyn_system__input_event_t *);






// independent input device, adstracted for both 
// midi and evdev devices, since both event devices 
// are treated the same way in gensyn
typedef struct gensyn_linux_input_device_t gensyn_linux_input_device_t;
struct gensyn_linux_input_device_t{
    // null if evdev device
    snd_rawmidi_t * midi;
    
    // // null if midi device 
    //libevdev * evdevice;
    
    // always a string but the format depends on the type of device
    gensyn_string_t * devicePath;
        
    // queue to receive events from
    ev_queue_t * events;
    
    // name identifier of the device
    gensyn_string_t * name;
    
    // description of the device, if avail
    gensyn_string_t * desc;
    
    // function that polls input for the device
    void (*update)(gensyn_linux_input_device_t *, int);
    
};




// State of all input
struct gensyn_linux_input_t {
    // of type gensyn_linux_input_t *
    gensyn_array_t * devices;
    
    ev_queue_t * events;
    
};








static void gensyn_linux_input_device_update__midi(gensyn_linux_input_device_t * dev, int devId) {
    int index = 0;
    uint8_t code;
    uint8_t status = 0;
    gensyn_system__input_event_t ev = {0};
    ev.deviceID = devId;
    
    while(snd_rawmidi_read(dev->midi, &code, 1)) {
        // status!
        if (code & 0b10000000) {
            status = code;
            ev.input = status;
        // data!
        } else {
            // since we always go to data 
            if (index++%2==0) {
                ev.inputData1 = code;
            } else {
                ev.inputData2 = code;
                ev_queue_push(dev->events, &ev);
                printf("new event: dev %d -> %d %d %d\n", 
                    ev.deviceID,
                    ev.input,
                    ev.inputData1,
                    ev.inputData2
                );                        
            }
        }
    }
}


gensyn_linux_input_t * gensyn_linux_input_create() {
    gensyn_linux_input_t * out = calloc(1, sizeof(gensyn_linux_input_t));
    out->events = ev_queue_create();
    out->devices = gensyn_array_create(sizeof(gensyn_linux_input_device_t*));
    return out;
}




// Attempts to get device information
int gensyn_system_input_get_device_info(
    gensyn_system_t *   s,
    int                 deviceID,
    gensyn_string_t *   name_out,
    gensyn_string_t *   desc_out
) {
    uint32_t len = gensyn_array_get_size(s->input->devices);
    if (deviceID < 0 || deviceID >= len) return 0;
    
    gensyn_string_set(name_out, gensyn_array_at(s->input->devices, gensyn_linux_input_device_t *, deviceID)->name);
    gensyn_string_set(desc_out, gensyn_array_at(s->input->devices, gensyn_linux_input_device_t *, deviceID)->desc);
    return 1;
}





// Requeries what devices are available. The total devices 
// found is returned
int gensyn_system_input_query_devices(gensyn_system_t * g) {
    gensyn_string_t * output = exec_capture_output(
        GENSYN_STR_CAST("amidi"),
        GENSYN_STR_CAST("-l")
    );
    
    if (!gensyn_string_get_length(output)) {
        gensyn_string_destroy(output);
        printf("Call to ALSA amidi to check midi instrument availability failed. Check to see that it's in your path and that it's installed. It is normally part of a full ALSA installation.\n");
        return 0;
    }
    
    const gensyn_string_t * token;
    const gensyn_string_t * lineSrc;
    gensyn_string_t * line = gensyn_string_create();
    
    
    int tokenIndex = 0;
    int lineCount = 0;
    
    uint32_t i;
    gensyn_string_t * midiPath = gensyn_string_create();
    gensyn_string_t * midiName = gensyn_string_create();
    int isFound;


    
    
    for(lineSrc = gensyn_string_chain_start(output, GENSYN_STR_CAST("\n"));
        !gensyn_string_chain_is_end(output);
        lineSrc = gensyn_string_chain_proceed(output)) {
        gensyn_string_set(line, lineSrc);
        if (lineCount == 0) {
            lineCount++;
            continue;
        }
    
        tokenIndex = 0;
        for(token = gensyn_string_chain_start(line, GENSYN_STR_CAST(" \t"));
            !gensyn_string_chain_is_end(line);
            token = gensyn_string_chain_proceed(line), tokenIndex++) {

            
            switch(tokenIndex) {
            case 0: break; // IO?

            case 1: // hardware path
                gensyn_string_set(midiPath, token);
                break; 

            case 2: // name first string
                gensyn_string_set(midiName, token);
                break;
                
            default: // name other parts if any. Rest of line is the name
                gensyn_string_concat(midiName, GENSYN_STR_CAST(" "));
                gensyn_string_concat(midiName, token);
                break;
            }
                
        }

        // assemble the device info
        for(i = 0; i < gensyn_array_get_size(g->input->devices); ++i) {
            if (gensyn_string_test_eq(gensyn_array_at(g->input->devices, gensyn_linux_input_device_t*, i)->name, midiName)) {
                gensyn_linux_input_device_t * dev = gensyn_array_at(g->input->devices, gensyn_linux_input_device_t*, i);
                
                
                // the device has changed paths, likely from disconnecting and reconnecting.
                // update the path so we dont lose the index
                if (!gensyn_string_test_eq(dev->devicePath, midiPath)) {
                    snd_rawmidi_close(dev->midi);
                    gensyn_string_set(dev->devicePath, midiPath);
                    snd_rawmidi_open(
                        &dev->midi, 
                        NULL,
                        gensyn_string_get_c_str(dev->devicePath),
                        0
                    );
                }
                
                
                break;
            }
            
        }
        
        // not found, so add it as a new device.
        if (i == gensyn_array_get_size(g->input->devices)) {
            gensyn_linux_input_device_t * dev = calloc(1, sizeof(gensyn_linux_input_device_t));
            dev->name = gensyn_string_clone(midiName);
            dev->desc = gensyn_string_clone(midiName);
            dev->devicePath = gensyn_string_clone(midiPath);
            dev->events = ev_queue_create();
            dev->update = gensyn_linux_input_device_update__midi;
            snd_rawmidi_open(
                &dev->midi, 
                NULL,
                gensyn_string_get_c_str(dev->devicePath),
                0
            );
            gensyn_array_push(g->input->devices, dev);
        }
    }
    
    gensyn_string_destroy(midiPath);
    gensyn_string_destroy(midiName);
    gensyn_string_destroy(line);
    
    return gensyn_array_get_size(g->input->devices);
}

// Poles and processes input from the user. This includes 
// midi events
void gensyn_system_input_update(gensyn_system_t * g) {
    uint32_t i;
    
    // todo: separate into a polling func?
    for(i = 0; i < gensyn_array_get_size(g->input->devices); ++i) {
        gensyn_linux_input_device_t * dev = gensyn_array_at(g->input->devices, gensyn_linux_input_device_t*, i);
        dev->update(dev, i);    
    }

    gensyn_system__input_event_t ev;    
    for(i = 0; i < gensyn_array_get_size(g->input->devices); ++i) {
        gensyn_linux_input_device_t * dev = gensyn_array_at(g->input->devices, gensyn_linux_input_device_t*, i);
        while(!ev_queue_empty(dev->events)) {
            ev = ev_queue_pop(dev->events);
            ev_queue_push(g->input->events, &ev);
        }        
    }
    
}


// Returns how many events are remaining after 
// calling this function
int gensyn_system_input_get_events(gensyn_system_t * g,
    gensyn_system__input_event_t * events,
    uint32_t                       eventsMax,
    uint32_t *                     eventsReceived
) {
    *eventsReceived = 0;
    uint32_t i;
    for(i = 0; i < eventsMax && !ev_queue_empty(g->input->events); ++i) {
        events[(*eventsReceived)++] = ev_queue_pop(g->input->events);
    }
    return !ev_queue_empty(g->input->events);
}


// Sends an input event onto the queue.
// Useful for simulating events externally
void gensyn_system_input_send_event(gensyn_system_t * g, const gensyn_system__input_event_t * ev) {
    ev_queue_push(g->input->events, ev);
}







void gensyn_system_usleep(uint32_t u) {
    usleep(u);
}


static pthread_t threadPool[0xff] = {0};
uint8_t threadPoolID = 0;

uint8_t gensyn_system_thread_create(gensyn_system_t * g, void * (*threadMain)(void *), void * userData) {
    pthread_create(
        threadPool+threadPoolID,
        NULL,
        threadMain,
        userData
    );
    return threadPoolID++;
}

void gensyn_system_thread_cancel(gensyn_system_t * s, uint8_t id) {
    assert(!"sorry not yet");
    
}






///////// utility implementation 

struct ev_queue_t {
    uint32_t allocSize;
    uint32_t ptr;
    gensyn_system__input_event_t * q;
    
};

// Creates a new event queue
static ev_queue_t * ev_queue_create() {
    ev_queue_t * out = calloc(1, sizeof(ev_queue_t));
    out->allocSize = 256;
    out->q = malloc(sizeof(gensyn_system__input_event_t)*out->allocSize);
}

// Destroys an event queue
static void ev_queue_destroy(ev_queue_t * q) {
    free(q->q);
    free(q);
}

// Pops an event from the queue
// If none left, object is empty
static gensyn_system__input_event_t ev_queue_pop(ev_queue_t * q) {
    if (q->ptr == 0) {
        gensyn_system__input_event_t out = {0};
        return out;
    }
    return q->q[q->ptr--];
}

// Returns whether the event queue is empty
static int ev_queue_empty(const ev_queue_t * q) {
    return q->ptr == 0;
}

// Pushes an evetn to the queue
static void ev_queue_push(ev_queue_t * q, const gensyn_system__input_event_t * ev) {
    if (q->ptr + 1 >= q->allocSize) {
        q->allocSize = 1.4*q->allocSize;
        q->q = realloc(q->q, q->allocSize*sizeof(gensyn_system__input_event_t));
    }
    q->q[q->ptr++] = *ev;
}









gensyn_string_t * exec_capture_output(const gensyn_string_t * prog, const gensyn_string_t * args) {
    gensyn_string_t * str = gensyn_string_create();
    gensyn_string_concat(str, prog);
    gensyn_string_concat_printf(str, " ");
    gensyn_string_concat(str, args);

    FILE * f = popen(gensyn_string_get_c_str(str), "r");
    
    gensyn_string_clear(str);
    int c;
    while(!feof(f)) {
        c = fgetc(f);
        gensyn_string_concat_printf(str, "%c", (char)c);
    }
    pclose(f);
    return str;
}






#endif

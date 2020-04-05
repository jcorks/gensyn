#include <gensyn/gensyn.h>
#include <gensyn/system.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main() {
    
    gensyn_t * g = gensyn_create();
    gensyn_system_t * env = gensyn_get_system(g);
    
    
    // query the devices. THe querying processes 
    // also updates which devices gensyn will listen from.
    int numDev = gensyn_system_input_query_devices(env);
    if (!numDev) {
        printf("No MIDI devices detected! Exiting...\n");
        exit(1);
    } else {
        printf("Number of MIDI devices listening: %d\n", numDev);
    }
    
    
    // get info on detected devices.
    int i;
    gensyn_string_t * name = gensyn_string_create();
    gensyn_string_t * desc = gensyn_string_create();
    for(i = 0; i < numDev; ++i) {
        gensyn_system_input_get_device_info(
            env,
            i,
            name,
            desc
        );
        
        printf(
            "Device %d:\nName: %s\nDesc: %s\n\n", 
            i,
            gensyn_string_get_c_str(name),
            gensyn_string_get_c_str(desc)
        );
    }

    
    // listen for events and report them.
    gensyn_system__input_event_t events[256];
    uint32_t numEventsReceived;
    while(1) {
        gensyn_system_input_update(env);
        while(gensyn_system_input_get_events(env, events, 256, &numEventsReceived)) {
            for(i = 0; i < numEventsReceived; ++i) {
                printf("EVENT: %d %d %d %d\n",
                    events[i].deviceID,
                    events[i].input,
                    events[i].inputData1,
                    events[i].inputData2
                );
            }
        }
    }
}

#include <gensyn/gensyn.h>
#include <gensyn/gate.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
 *  GenSyn - CLI
 * 
 *  Can be used to produce synth samples from gensyn using 
 *  ECMA scripting to build / load  / confiugure the synth 
 * 
 *  Using the "@" command, a raw PCM file can be produced
 *  The format can be configured using the values below.
 * 
 */




#define SAMPLERATE    44100
#define BUFFERSIZE    256
#define DURATION_SEC  10



static void write_output_pcm(gensyn_t * g, const char * output) {
    FILE * f = fopen(output, "wb");
    if (!f) {
        printf("Cannot write output waveform to %s\n", output);
        return;
    }
    
    float    * buffer     = calloc(BUFFERSIZE, sizeof(float));

    uint32_t i, n;
    for(i = 0; i < SAMPLERATE * DURATION_SEC; i+=BUFFERSIZE) {
        gensyn_gate_run(
            gensyn_get_output_gate(g),
            buffer,
            BUFFERSIZE,
            SAMPLERATE
        );
        
        fwrite(buffer, BUFFERSIZE, sizeof(float), f);        
    }
    free(buffer);
    fclose(f);
    printf("Wrote waveform (32bit float [-1, 1])to %s\n", output);
}


int main() {
    
    gensyn_t * g = gensyn_create();
    
    
    printf("\nGenSyn CLI\n");
    printf("Johnathan Corkery, 2020\n\n");

    
    printf("Run gensyn.help() to see more information on usage.\n");

    
    
    
    gensyn_send_command(
        g,
        GENSYN_STR_CAST(
            "var testSynth = function() {\n"
            "   var input = gensyn.gate.add('Simple_Input', 'TestInput');\n"
            "   var wave  = gensyn.gate.add('Sine_Wave', 'TestWave');\n"
            "   var osc   = gensyn.gate.add('Simple_LFO', 'TestLFO');\n"
            "   var adder = gensyn.gate.add('Adder', 'TestAdder');\n"
            
            "   input.setParam('value', -0.8925527503477801);\n"
            "   osc.setParam('max', 0.01);\n"
            "   osc.setParam('hz', 4);\n"
            
            "   osc.connectTo('input0', adder);\n"
            "   input.connectTo('input1', adder);\n"

            "   adder.connectTo('pitch', wave);\n"
            "   wave.connectTo('waveform', gensyn.getOutput());\n"
            "}; testSynth();\n"

            
            
        )
    );

    char buffer[4096];
    while(1) {
        printf("$ "); fflush(stdout);
        fgets(buffer, 4096, stdin);


        // output command
        if (buffer[0] == '@') {
            // remove newline.
            buffer[strlen(buffer)-1] = 0;
            write_output_pcm(g, buffer+1);
        } else {
            
            printf("%s\n", 
                gensyn_string_get_c_str(
                    gensyn_send_command(g, GENSYN_STR_CAST(buffer))
                )
            );
        }
    }
}

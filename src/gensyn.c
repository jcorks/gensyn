#include <gensyn/gensyn.h>
#include <gensyn/gate.h>
#include <gensyn/table.h>
#include <gensyn/sample.h>
#include <gensyn/system.h>
#include <gensyn/ring.h>
#include "extern/duktape.h"
#include "extern/srgs.h"



///////
#include "gates/output.h"
#include "gates/sine_wave.h"
#include "gates/simple_input.h"
#include "gates/adder.h"
#include "gates/lfo.h"
#include "gates/glider.h"
///////
 
struct gensyn_t {
    gensyn_gate_t * output;

    gensyn_table_t * gates;
    gensyn_table_t * fnCmd;

    gensyn_system_t * sys;
    
    duk_context * ecma;
    
    int x;
    int y;
    
    int texture;
    gensyn_string_t * result;
    
    gensyn_table_iter_t * tableIter;
    
    
    // gates sent to the input thread
    gensyn_ring_t * commandAdd;
    gensyn_ring_t * commandRemove;
    
    // all gates that accept 
    gensyn_array_t * inputGates;
};

static gensyn_table_t * ecmaToInstance = NULL;

// Starts the input loop for the system.
void gensyn_start_input_loop(gensyn_t * t);


// Registers all built-in gate types.
static void register_gate_types();


 
const char * initialjs = 
"gensyn = (function() {\n"
"    return {\n"
"        'gate' : {\n"
             // creates a new gate object
"            'add' : function(gateType, gateName) {\n"
"                if (gateType == '' || gateName == '') throw new Error('Neither type nor name may be NULL.');\n"
"                var result = __gensyn_c_native('gate-add', gateType, gateName);\n"
"                if (result != '') throw new Error(result);\n"
"                return this.get(gateName);\n"
"            },\n"
             // gets an object referring to a real gate
"            'get' : function(gateName) {\n"
"                if (__gensyn_c_native('gate-check', gateName) != '') {\n"
"                    throw new Error(gateName + ' does not refer to a gate!');\n"
"                }\n"                
"                return {\n"
"                    'name' : gateName,\n"
"                    'remove' : function() {\n"
"                        __gensyn_c_native('gate-remove', gateName);\n"
"                    },\n"
"                    'summary' : function() {\n"
"                        return __gensyn_c_native('gate-summary', gateName);\n"
"                    },\n"
"                    'connectTo' : function(otherConnection, otherGateObject) {\n"
"                        var result = __gensyn_c_native('gate-connect', gateName, otherConnection, otherGateObject.name);\n"
"                        if (result != '') {\n"
"                            throw new Error(result);\n"
"                        }\n"
"                    },\n"
"                    'setParam' : function(paramName, value) {\n"
"                        __gensyn_c_native('gate-set-param', gateName, paramName, value);\n"
"                    },\n"
"                    'getParam' : function(paramName) {\n"
"                        return Number.parse(__gensyn_c_native('gate-get-param', gateName, paramName));\n"
"                    },\n"
"                }\n"
"            },\n"
            // returns an array of all the names of gates
"            'list' : function() {\n"
"                var listRaw = __gensyn_c_native('gate-list');\n"
"                return listRaw.split('\\n');\n"
"            }\n"
"        },\n"
        // reduces the state of all gates into a JSON object.
"        saveState : function() {\n"
"            throw new Error('Havent implemented this yet');\n"
"        },\n"
        // Loads the state of gensyn from a JSON object.
"        loadState : function(state) {\n"
"            throw new Error('Havent implemented this yet');\n"
"        },\n"
        // returns the default output object that will receive the waveform
"        getOutput : function() {\n"
"            return this.gate.get('output');\n"
"        },\n"
"        help : function() {\n"
"            return __gensyn_c_native('help');\n"
"        },\n"
"        noteToPitch : {\n"
"            'C0'  : 0,\n" 
"            'C#0' : -0.9997539859974917,\n" 
"            'Db0' : -0.9997539859974917,\n" 
"            'D'   : -0.9994927546340034,\n" 
"            'D#0' : -0.9992137696827054,\n" 
"            'Eb0' : -0.9992137696827054,\n" 
"            'E0'  : -0.9989221035972573,\n" 
"            'F0'  : -0.9986101476971695,\n" 
"            'F#0' : -0.9982829744361017,\n" 
"            'Gb0' : -0.9982829744361017,\n" 
"            'G0'  : -0.997932975133564,\n" 
"            'G#0' : -0.9975626860163865,\n" 
"            'Ab0' : -0.9975626860163865,\n" 
"            'A0'  : -0.9971721070845693,\n" 
"            'A#0' : -0.996756165884452,\n" 
"            'Ab0' : -0.996756165884452,\n" 
"            'B0'  : -0.9963173986428651,\n" 


"            'C1'  : -0.9958532691329782,\n" 
"            'C#1' : -0.9953587049011315,\n" 
"            'Db1' : -0.9953587049011315,\n" 
"            'D1'  : -0.9948362421741551,\n" 
"            'D#1' : -0.9942833447252188,\n" 
"            'Eb1' : -0.9942833447252188,\n" 
"            'E1'  : -0.9936974763274928,\n" 
"            'F1'  : -0.9930761007541471,\n" 
"            'F#1' : -0.9924166817783515,\n" 
"            'Gb1' : -0.9924166817783515,\n" 
"            'G1'  : -0.9917192194001063,\n" 
"            'G#1' : -0.9909811773925813,\n" 
"            'Ab1' : -0.9909811773925813,\n" 
"            'A1'  : -0.9901974833021167,\n" 
"            'A#1' : -0.9893681371287122,\n" 
"            'Bb1' : -0.9893681371287122,\n" 
"            'B1'  : -0.9884880664187082,\n" 

"            'C2'  : -0.9875572711721046,\n" 
"            'C#2' : -0.9865706789352413,\n" 
"            'Db2' : -0.9865706789352413,\n" 
"            'D2'  : -0.9855257534812883,\n" 
"            'D#2' : -0.9844199585834159,\n" 
"            'Eb2' : -0.9844199585834159,\n" 
"            'E2'  : -0.9832456855611339,\n" 
"            'F2'  : -0.9820029344144423,\n" 
"            'F#2' : -0.9806866326896813,\n" 
"            'Gb2' : -0.9806866326896813,\n" 
"            'G2'  : -0.9792917079331908,\n" 
"            'G#2' : -0.9778130876913108,\n" 
"            'Ab2' : -0.9778130876913108,\n" 
"            'A2'  : -0.9762482357372114,\n" 
"            'A#2' : -0.9745895433904027,\n" 
"            'Bb2' : -0.9745895433904027,\n" 
"            'B2'  : -0.9728319381972246,\n" 

"            'C3'  : -0.9709703477040172,\n" 
"            'C#3' : -0.9689971632302906,\n" 
"            'Db3' : -0.9689971632302906,\n" 
"            'D3'  : -0.9669073123223848,\n" 
"            'D#3' : -0.9646931862998099,\n" 
"            'Eb3' : -0.9646931862998099,\n" 
"            'E3'  : -0.9623471764820759,\n" 
"            'F3'  : -0.9598616741886927,\n" 
"            'F#3' : -0.9572265345123406,\n" 
"            'Gb3' : -0.9572265345123406,\n" 
"            'G3'  : -0.9544366849993596,\n" 
"            'G#3' : -0.9514819807424296,\n" 
"            'Ab3' : -0.9514819807424296,\n" 
"            'A3'  : -0.948349740607401,\n" 
"            'A#3' : -0.9450323559137835,\n" 
"            'Bb3' : -0.9450323559137835,\n" 
"            'B3'  : -0.9415171455274274,\n" 


"            'C4'  : -0.9377914283141827,\n" 
"            'C#4' : -0.9338475955935595,\n" 
"            'Db4' : -0.9338475955935595,\n" 
"            'D4'  : -0.929667893777748,\n" 
"            'D#4' : -0.925237105505768,\n" 
"            'Eb4' : -0.925237105505768,\n" 
"            'E4'  : -0.9205450858702999,\n" 
"            'F4'  : -0.9155740812835337,\n" 
"            'F#4' : -0.9103088743844895,\n" 
"            'Gb4' : -0.9103088743844895,\n" 
"            'G4'  : -0.9047266391316974,\n" 
"            'G#4' : -0.8988172306178376,\n" 
"            'Ab4' : -0.8988172306178376,\n" 
"            'A4'  : -0.8925527503477801,\n" 
"            'A#4' : -0.8859179809605452,\n" 
"            'Bb4' : -0.8859179809605452,\n" 
"            'B4'  : -0.878887560187833,\n" 


"            'C5'  : -0.8714386619881735,\n" 
"            'C#5' : -0.8635459240932672,\n" 
"            'Db5' : -0.8635459240932672 ,\n" 
"            'D5'  : -0.855186520461644,\n" 
"            'D#5' : -0.8463300163713442,\n" 
"            'Eb5' : -0.8463300163713442,\n" 
"            'E5'  : -0.836945977100408,\n" 
"            'F5'  : -0.8270014317000455,\n" 
"            'F#5' : -0.8164684816751271,\n" 
"            'Gb5' : -0.8164684816751271,\n" 
"            'G5'  : -0.8053090836232029,\n" 
"            'G#5' : -0.7934851941418233,\n" 
"            'Ab5' : -0.7934851941418233,\n" 
"            'A5'  : -0.7809587698285384,\n" 
"            'A#5' : -0.7676866948272385,\n" 
"            'Bb5' : -0.7676866948272385,\n" 
"            'B5'  : -0.7536258532818141,\n" 


"            'C6'  : -0.7387305931093253,\n" 
"            'C#6' : -0.7229476535463426,\n" 
"            'Db6' : -0.7229476535463426,\n" 
"            'D6'  : -0.7062263100562661,\n" 
"            'D#6' : -0.6885107656488365,\n" 
"            'Eb6' : -0.6885107656488365,\n" 
"            'E6'  : -0.669742687106964,\n" 
"            'F6'  : -0.6498586687598992,\n" 
"            'F#6' : -0.6287902324832324,\n" 
"            'Gb6' : -0.6287902324832324,\n" 
"            'G6'  : -0.6064714363793839,\n" 
"            'G#6' : -0.5828236574166247,\n" 
"            'Ab6' : -0.5828236574166247,\n" 
"            'A6'  : -0.5577708087900549,\n" 
"            'A#6' : -0.5312266587874552,\n" 
"            'Bb6' : -0.5312266587874552,\n" 
"            'B6'  : -0.5031075119234363,\n" 

"            'C7'  : -0.47331445535162875,\n" 
"            'C#7' : -0.4417485762256632,\n" 
"            'Db7' : -0.4417485762256632,\n" 
"            'D7'  : -0.4083058892455105,\n" 
"            'D#7' : -0.37287480043065124,\n" 
"            'Eb7' : -0.37287480043065124,\n" 
"            'E7'  : -0.33533864334690633,\n" 
"            'F7'  : -0.29556807042594657,\n" 
"            'F#7' : -0.2534337340994428,\n" 
"            'Gb7' : -0.2534337340994428,\n" 
"            'G7'  : -0.2087961418917461,\n" 
"            'G#7' : -0.16150058396622757,\n" 
"            'Ab7' : -0.16150058396622757,\n" 
"            'A7'  : -0.11139488671308795,\n" 
"            'A#7' : -0.05830912293471868,\n" 
"            'Bb7' : -0.05830912293471868,\n" 
"            'B7'  : -0.0020657567530208976,\n" 


"            'C8'  : 0.05752035639059416,\n" 
"            'C#8' : 0.12064957841569512,\n" 
"            'Db8' : 0.12064957841569512,\n" 
"            'D8'  : 0.1875324161491707,\n" 
"            'D#8' : 0.2583945937788892,\n" 
"            'Eb8' : 0.2583945937788892,\n" 
"            'E8'  : 0.3334694441732091,\n" 
"            'F8'  : 0.4130080537882985,\n" 
"            'F#8' : 0.49727672644130605,\n" 
"            'G8'  : 0.5865569833103594,\n" 
"            'G#8' : 0.6811455629345666,\n" 
"            'Ab8' : 0.6811455629345666,\n" 
"            'A8'  : 0.7813569574408457,\n" 
"            'A#8' : 0.8875284849975842,\n" 
"            'Bb8' : 0.8875284849975842,\n" 
"            'B8'  : 1.0,\n" 
"        }\n"
"    }\n"
"})();\n";




 
// native function called by the ecma context
static duk_ret_t gensyn_ecma_c_native(duk_context * ctx);

// string for the context
static void gensyn_ecma_c_err_handler(void * context, const char * str);

// Raw commands. These are the functions run in the ECMAscript context 
// as entry points into the C runtime.

// all commands use this format.
typedef void (*gensyn_command_fn)(gensyn_t *, gensyn_string_t **, int, gensyn_string_t *);



// help [command]
//  -   Displays help. With no command given, it will display available commands
static void gensyn_command__help(gensyn_t *, gensyn_string_t **, int, gensyn_string_t *);

// gate-list
//  -   Lists all added gates by name separated by newlines
static void gensyn_command__gate_list(gensyn_t *, gensyn_string_t **, int, gensyn_string_t *);


// gate-check nameOfGate
//  -  Checkes to see if a gate exists. If nameOfGate refers to a gate, then 
//     the empty string is returned. Else, a non-empty string is returned.
static void gensyn_command__gate_check(gensyn_t *, gensyn_string_t **, int, gensyn_string_t *);


// gate-add typeOfGate nameForGate
//  -   Adds a new gate with the given type and name. The name is 
//      used to refer to this gate instance. If an error occurs, it will return the error
//      If successful, will return an empty string.
static void gensyn_command__gate_add(gensyn_t *, gensyn_string_t **, int, gensyn_string_t *);

// gate-summary idForGate
//  -   Gives a detailed summary of a gate 
// 
static void gensyn_command__gate_summary(gensyn_t *, gensyn_string_t **, int, gensyn_string_t *);

// gate-connect fromGateID connectionName toGateID
//  -   connects a gate to another gate at the specified slot. You can see 
//      the gate summary for info on slots. If successful, returns the empty string.
static void gensyn_command__gate_connect(gensyn_t *, gensyn_string_t **, int, gensyn_string_t *);

// gate-disconnect fromGateID connectionName toGateID 
//  -   disconnects a gate to another gate at the specified slot. You can see 
//      the gate summary for info on slots. If successful, returns the empty string.
static void gensyn_command__gate_disconnect(gensyn_t *, gensyn_string_t **, int, gensyn_string_t *);


// gate-get-param gateID parameterName
//  -   gets the given parameter value 
static void gensyn_command__gate_get_param(gensyn_t *, gensyn_string_t **, int, gensyn_string_t *);

// gate-set-param gateID parameterName paramValue
//  -   sets the given parameter value 
static void gensyn_command__gate_set_param(gensyn_t *, gensyn_string_t **, int, gensyn_string_t *);


// runs the given command
static void gensyn_command_run_internal(gensyn_t *, gensyn_string_t **, int, gensyn_string_t *);


gensyn_t * gensyn_create() {
    gensyn_t * out = calloc(1, sizeof(gensyn_t));
    out->gates = gensyn_table_create_hash_gensyn_string();
    out->fnCmd = gensyn_table_create_hash_gensyn_string();
    out->result = gensyn_string_create();


    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("help"),           gensyn_command__help);
    //gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("error"),          gensyn_command__error);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate-list"),      gensyn_command__gate_list);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate-check"),     gensyn_command__gate_check);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate-add"),       gensyn_command__gate_add);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate-summary"),   gensyn_command__gate_summary);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate-connect"),   gensyn_command__gate_connect);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate-disconnect"),gensyn_command__gate_disconnect);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate-get-param"), gensyn_command__gate_get_param);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate-set-param"), gensyn_command__gate_set_param);

    
    out->tableIter = gensyn_table_iter_create();
    out->ecma = duk_create_heap(NULL, NULL, NULL, out, gensyn_ecma_c_err_handler);
    duk_push_c_function(out->ecma, gensyn_ecma_c_native, DUK_VARARGS);
    duk_put_global_string(out->ecma, "__gensyn_c_native");

    if (!ecmaToInstance) {
        ecmaToInstance = gensyn_table_create_hash_pointer();
    }
    gensyn_table_insert(ecmaToInstance, out->ecma, out);
    
    register_gate_types();

    
    out->output = gensyn_create_named_gate(
        out, 
        GENSYN_STR_CAST("GenSyn_Output"), 
        GENSYN_STR_CAST("output")
    );
    
    
    const gensyn_string_t * in = gensyn_send_command(out, GENSYN_STR_CAST(initialjs));
    if (gensyn_string_get_length(in)) {
        printf("%s\n", gensyn_string_get_c_str(in));
    }
    
    out->sys = gensyn_system_create();
    
    
    out->commandAdd    = gensyn_ring_create(sizeof(gensyn_gate_t*), 1024);
    out->commandRemove = gensyn_ring_create(sizeof(gensyn_gate_t*), 1024);
    out->inputGates    = gensyn_array_create(sizeof(gensyn_gate_t*));
    
    gensyn_start_input_loop(out);
    return out;
}


// Gets the main output gate.
gensyn_gate_t * gensyn_get_output_gate(const gensyn_t * g) {
    return (gensyn_gate_t *)g->output;
}


const gensyn_string_t * gensyn_send_command(const gensyn_t * g, const gensyn_string_t * str) {
    duk_push_string(g->ecma, gensyn_string_get_c_str(str));
    duk_peval(g->ecma);
    gensyn_string_clear(g->result);
    gensyn_string_concat_printf(g->result, "%s", duk_safe_to_string(g->ecma, -1));
    duk_pop(g->ecma);
    
    return g->result;
}
 
gensyn_gate_t * gensyn_create_named_gate(
    gensyn_t * g, 
    const gensyn_string_t * type,
    const gensyn_string_t * name
) {
    // must be uniquely named
    if (gensyn_get_named_gate(g, name)) {
        return NULL;
    }
    
    
    gensyn_gate_t * gate = gensyn_gate_create(g, type);

    // unrecognized name
    if (!gate) {
        return NULL;
    }

    gensyn_table_insert(g->gates, name, gate);
    return gate;
}



gensyn_gate_t * gensyn_get_named_gate(const gensyn_t * g, const gensyn_string_t * name) {
    return gensyn_table_find(g->gates, name);
}



void gensyn_destroy_named_gate(const gensyn_t * g, const gensyn_string_t * name) {
    gensyn_gate_t * gate = gensyn_get_named_gate(g, name);
    if (!gate) {
        return;
    }
    
    gensyn_table_remove(g->gates, name);
    gensyn_gate_destroy(gate);
}


void gensyn_generate_waveform(
    gensyn_t * g, 
    gensyn_sample_t * samplesOut,
    uint32_t sampleCount,
    float   sampleRate
) {
    uint32_t i;
    
    // reset active status.
    // this will get updated by gate_run 
    for(gensyn_table_iter_start(g->tableIter, g->gates);
        !gensyn_table_iter_is_end(g->tableIter);
        gensyn_table_iter_proceed(g->tableIter)) {
        gensyn_gate_reset_is_active(gensyn_table_iter_get_value(g->tableIter));
    }
    gensyn_gate_run(
        gensyn_get_output_gate(g),
        samplesOut,
        sampleCount,
        sampleRate
    );

}

void gensyn_set_origin(gensyn_t * g, int x, int y) {
    g->x = x;
    g->y = y;
}

void gensyn_render_scene(gensyn_t * g) {
    
}

int gensyn_get_framebuffer_texture(gensyn_t * g) {
    
}

gensyn_system_t * gensyn_get_system(gensyn_t * g) {
    return g->sys;
}


/////////////////// statics 


void register_gate_types() {
    gensyn_gate_add__gensyn_output();
    gensyn_gate_add__sine_wave();
    gensyn_gate_add__simple_input();
    gensyn_gate_add__lfo();
    gensyn_gate_add__adder();
    gensyn_gate_add__glider();
}



static duk_ret_t gensyn_ecma_c_native(duk_context * ctx) {
    int n = duk_get_top(ctx);
    int i;
    gensyn_t * inst = gensyn_table_find(ecmaToInstance, ctx);
    const gensyn_string_t * args[n];
    
    for(i = 0; i < n; ++i) {
        args[i] = GENSYN_STR_CAST(duk_to_string(ctx, i));
    }
    
    gensyn_string_t * out = gensyn_string_create();
    gensyn_command_run_internal(inst, (gensyn_string_t**)args, n, out);
    duk_push_string(ctx, gensyn_string_get_c_str(out));
    gensyn_string_destroy(out);

    // pushes string on the stack
    return 1;
}

static void gensyn_ecma_c_err_handler(void * context, const char * str) {
    gensyn_t * ctx = context;
    printf("Fatal uncaught error in GenSyn %p: %s\n", context, str);
    
}


static void gensyn_command_run_internal(
    gensyn_t *          g, 
    gensyn_string_t **  args, 
    int                 argc, 
    gensyn_string_t *   output
) {
    gensyn_command_fn fn = gensyn_table_find(g->fnCmd, args[0]);
    if (!fn) {
        gensyn_string_concat_printf(output, "Unrecognized action \"%s\".", gensyn_string_get_c_str(args[0]));
        return;
    }
    
    fn(g, args+1, argc-1, output);
}







//// actual native commands

// help [command]
//  -   Displays help. With no command given, it will display available commands
static void gensyn_command__help(gensyn_t * ctx, gensyn_string_t ** args, int argc, gensyn_string_t * output) {
    gensyn_string_concat_printf(
        output,
        "GenSyn Command Processor.\n"
        "Johnathan Corkery, 2020\n\n"
        
        "Help comming soon <3"
    );
}

// gate-list
//  -   Lists all added gates by name separated by newlines
static void gensyn_command__gate_list(
    gensyn_t *          ctx, 
    gensyn_string_t **  args, 
    int                 argc, 
    gensyn_string_t *   output
) {
    gensyn_table_iter_t * iter = gensyn_table_iter_create();
    for( gensyn_table_iter_start(iter, ctx->gates);
        !gensyn_table_iter_is_end(iter);
         gensyn_table_iter_proceed(iter)) {
        gensyn_string_concat(output, gensyn_table_iter_get_key(iter));
        gensyn_string_concat_printf(output, "\n");
    }
}


// gate-check nameOfGate
//  -  Checkes to see if a gate exists. If nameOfGate refers to a gate, then 
//     the empty string is returned. Else, a non-empty string is returned.
static void gensyn_command__gate_check(
    gensyn_t *          ctx, 
    gensyn_string_t **  args, 
    int                 argc, 
    gensyn_string_t *   output
) {
    if (argc == 0) {
        gensyn_string_concat_printf(output, "No gate name given.");
        return;
    }
    
    if (!gensyn_get_named_gate(ctx, args[0])) {
        gensyn_string_concat_printf(output, "No gate with the given name");
    }
    
}



// gate-add typeOfGate nameForGate
//  -   Adds a new gate with the given type and name. The name is 
//      used to refer to this gate instance. If an error occurs, it will return the error
//      If successful, will return an empty string.
static void gensyn_command__gate_add(
    gensyn_t *          ctx, 
    gensyn_string_t **  args, 
    int                 argc, 
    gensyn_string_t *   output
) {
    if (argc != 2) {
        gensyn_string_concat_printf(output, "Insufficient arguments");
        return;
    }
        
    
    if (!gensyn_create_named_gate(
        ctx,
        args[0],
        args[1]
    )) {
        gensyn_string_concat_printf(output, "Could not create gate.");
    }
}


static const char * gensyn_gate_type_to_cstr(const gensyn_gate_t * g) {
    switch(gensyn_gate_get_type(g)) {
      case GENSYN_GATE__TYPE__INPUT:     return "InputGate";
      case GENSYN_GATE__TYPE__OUTPUT:    return "OutputGate";
      case GENSYN_GATE__TYPE__TRANSFORM: return "TransformGate";
      case GENSYN_GATE__TYPE__NULL:      return "InertGate";
    }
    return "";
}


// gate-summary idForGate
//  -   Gives a detailed summary of a gate 
// 
static void gensyn_command__gate_summary(
    gensyn_t *          ctx, 
    gensyn_string_t **  args, 
    int                 argc, 
    gensyn_string_t *   output
) {
    if (argc < 1) {
        gensyn_string_concat_printf(output, "Insufficient arguments");
        return;        
    }
    gensyn_gate_t * g = gensyn_get_named_gate(ctx, args[0]);
    if (!g) {
        gensyn_string_concat_printf(output, "Unrecognized gate name.");
        return;
    }
    gensyn_string_concat_printf(
        output, 
        
        "ID:     %s\n"
        "Class:  %s\n"
        "Type:   %s\n\n"
        "Description:\n\%s\n",
        
        
        gensyn_string_get_c_str(args[0]),
        gensyn_string_get_c_str(gensyn_gate_get_class(g)),
        gensyn_gate_type_to_cstr(g),
        gensyn_string_get_c_str(gensyn_gate_get_description(g))
    );

    
    gensyn_string_concat_printf(output, "\nInputs:\n");
    int i;
    const gensyn_array_t * names = gensyn_gate_get_in_names(g);
    for(i = 0; i < gensyn_array_get_size(names); ++i) {
        gensyn_gate_t * from = gensyn_gate_get_in_connection(
            g, 
            gensyn_array_at(
                names,
                gensyn_string_t *,
                i
            )
        ); 

        
        gensyn_string_concat_printf(output, "   - ");
        gensyn_string_concat(
            output,
            gensyn_array_at(
                names,
                gensyn_string_t *,
                i
            )
        );
        if (from)
            gensyn_string_concat_printf(output, " -> Connected\n");
        else 
            gensyn_string_concat_printf(output, "\n");
    }
    
    
    
    gensyn_string_concat_printf(
        output, 
        "\nParameters:\n"
    );
    names = gensyn_gate_get_param_names(g);
    for(i = 0; i < gensyn_array_get_size(names); ++i) {
        const gensyn_string_t * name = gensyn_array_at(names, gensyn_string_t *, i);
        float val = gensyn_gate_get_parameter(g, name);
        gensyn_string_concat_printf(output, "   - %s : %f\n", gensyn_string_get_c_str(name), val);
    }

    
    
    
}
    
// gate-connect fromGateID connectionName toGateID 
//  -   connects a gate to another gate at the specified slot. You can see 
//      the gate summary for info on slots. If successful, returns the empty string.
static void gensyn_command__gate_connect(
    gensyn_t *          ctx, 
    gensyn_string_t **  args, 
    int                 argc, 
    gensyn_string_t *   output
) {
    if(argc < 3) {
        gensyn_string_concat_printf(output, "Insufficient arguments");
        return;        
    }
    
    gensyn_gate_t * from = gensyn_get_named_gate(ctx, args[0]);
    gensyn_gate_t * to   = gensyn_get_named_gate(ctx, args[2]);
    
    if (!from) {
        gensyn_string_concat_printf(output, "Unrecognized gate name.");
        return;
    }
    if (!to) {
        gensyn_string_concat_printf(output, "Unrecognized gate name.");
        return;
    }

    gensyn_gate_connect(from, args[1], to);
}

// gate-disconnect fromGateID connectionName toGateID 
//  -   disconnects a gate to another gate at the specified slot. You can see 
//      the gate summary for info on slots. If successful, returns the empty string.
static void gensyn_command__gate_disconnect(
    gensyn_t *          ctx, 
    gensyn_string_t **  args, 
    int                 argc, 
    gensyn_string_t *   output
) {
    if(argc < 3) {
        gensyn_string_concat_printf(output, "Insufficient arguments");
        return;        
    }
    
    gensyn_gate_t * from = gensyn_get_named_gate(ctx, args[0]);
    gensyn_gate_t * to   = gensyn_get_named_gate(ctx, args[2]);
    
    if (!from) {
        gensyn_string_concat_printf(output, "Unrecognized gate name.");
        return;
    }
    if (!to) {
        gensyn_string_concat_printf(output, "Unrecognized gate name.");
        return;
    }

    gensyn_gate_disconnect(from, args[1], to);
}


// gate-get-param gateID parameterName
//  -   gets the given parameter value 
static void gensyn_command__gate_get_param(
    gensyn_t *          ctx, 
    gensyn_string_t **  args, 
    int                 argc, 
    gensyn_string_t *   output
) {
    if (argc < 2) {
        gensyn_string_concat_printf(output, "%f", 0.f);
        return;                
    }

    gensyn_gate_t * g = gensyn_get_named_gate(ctx, args[0]);

    gensyn_string_concat_printf(
        output, 
        "%f", 
        gensyn_gate_get_parameter(
            g,
            args[1] 
        )
    );
}

// gate-set-param gateID parameterName paramValue
//  -   sets the given parameter value 
static void gensyn_command__gate_set_param(
    gensyn_t *          ctx, 
    gensyn_string_t **  args, 
    int                 argc, 
    gensyn_string_t *   output
) {
    if (argc < 3) {
        gensyn_string_concat_printf(output, "%f", 0.f);
        return;
    }

    gensyn_gate_t * g = gensyn_get_named_gate(ctx, args[0]);

    gensyn_gate_set_parameter(
        g,
        args[1],
        atof(gensyn_string_get_c_str(args[2]))
    );
}






///// input loop stuff

static void gensyn_input_loop_thread__process_event(
    gensyn_t * g,
    const gensyn_system__input_event_t * event 
) {
    
    printf("EVENT: %d %d %d %d\n",
        event->deviceID,
        event->input,
        event->inputData1,
        event->inputData2
    );
    uint32_t i;
    for(i = 0; i < gensyn_array_get_size(g->inputGates); ++i) {
        gensyn_gate_send_event(
            gensyn_array_at(g->inputGates, gensyn_gate_t *, i),
            event
        );
    }
}


static void * gensyn_input_loop_thread__main(void * gSrc) {
    #define MAX_EVENTS_PER_ITER 128
    #define ITER_WAIT_TIME_MS   1
    gensyn_system__input_event_t events[MAX_EVENTS_PER_ITER];
    


    
    
    gensyn_t * g = gSrc;
    gensyn_system_t * sys = gensyn_get_system(g);
    
    time_t timeNow = 0;
    uint32_t count;
    for(;;) {
        // remove all old gates
        while (gensyn_ring_has_pending(g->commandRemove)) {
            gensyn_gate_t * gate = gensyn_ring_pop(g->commandRemove, gensyn_gate_t *);
            uint32_t i;
            for(i = 0; i < gensyn_array_get_size(g->inputGates); ++i) {
                if (gate == gensyn_array_at(g->inputGates, gensyn_gate_t *, i)) {
                    gensyn_array_remove(g->inputGates, i);
                    break;
                }
            }
        }
        
        // get all new gates 
        while (gensyn_ring_has_pending(g->commandAdd)) {
            gensyn_gate_t * gate = gensyn_ring_pop(g->commandAdd, gensyn_gate_t *);
            gensyn_array_push(g->inputGates, gate);
        }

        
        

        // priodically check the device state
        if (timeNow != time(NULL)) {
            gensyn_system_input_query_devices(sys);
            timeNow = time(NULL);
        }
        
        gensyn_system_input_update(sys);

        while(gensyn_system_input_get_events( 
            sys,
            events,
            MAX_EVENTS_PER_ITER,
            &count)) {
            
            uint32_t i;
            for(i = 0; i < count; ++i) {
                gensyn_input_loop_thread__process_event(g, events+i);
            }            
        }
        
        gensyn_system_usleep(1000);
    }
}


void gensyn_start_input_loop(gensyn_t * t) {
    gensyn_system_thread_create(t->sys, gensyn_input_loop_thread__main, t);
}
















#include <gensyn/gensyn.h>
#include <gensyn/gate.h>
#include <gensyn/table.h>

#include "extern/duktape.h"
#include "extern/srgs.h"



///////
#include "gates/output.h"

///////

struct gensyn_t {
    gensyn_gate_t * output;

    gensyn_table_t * gates;
    gensyn_table_t * fnCmd;
    
    duk_context * ecma;
    
    int x;
    int y;
    
    int texture;
    srgs_t * graphics;
    gensyn_string_t * result;
};

static gensyn_table_t * ecmaToInstance = NULL;



// Registers all built-in gate types.
static void register_gate_types();








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
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate_add"),       gensyn_command__gate_add);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate_summary"),   gensyn_command__gate_summary);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate_connect"),   gensyn_command__gate_connect);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate_disconnect"),gensyn_command__gate_disconnect);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate_get_param"), gensyn_command__gate_get_param);
    gensyn_table_insert(out->fnCmd, GENSYN_STR_CAST("gate_set_param"), gensyn_command__gate_set_param);

    
    
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
        GENSYN_STR_CAST("GenSyn Output"), 
        GENSYN_STR_CAST("output")
    );
    
    return out;
}


// Gets the main output gate.
gensyn_t * gensyn_get_output_gate(const gensyn_t * g) {
    return (gensyn_t *)g->output;
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


void gensyn_set_origin(gensyn_t * g, int x, int y) {
    g->x = x;
    g->y = y;
}

void gensyn_render_scene(gensyn_t * g) {
    
}

int gensyn_get_framebuffer_texture(gensyn_t * g) {
    
}




/////////////////// statics 


void register_gate_types() {
    gensyn_gate_add__gensyn_output();
    
}



static duk_ret_t gensyn_ecma_c_native(duk_context * ctx) {
    int n = duk_get_top(ctx);
    int i;
    gensyn_t * inst = gensyn_table_find(ecmaToInstance, ctx);
    const gensyn_string_t * args[n];
    
    for(i = 0; i < n; ++i) {
        args[n] = GENSYN_STR_CAST(duk_to_string(ctx, i));
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
        
        "ID:     %s\n",
        "Class:  %s\n",
        "Type:   %s\n",
        "Description:\n\n%s\n",
        
        
        gensyn_string_get_c_str(args[0]),
        gensyn_string_get_c_str(gensyn_gate_get_class(g)),
        gensyn_gate_type_to_cstr(g),
        gensyn_string_get_c_str(gensyn_gate_get_description(g))
    );

    
    gensyn_string_concat_printf(output, "Inputs:\n");
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

        
        gensyn_string_concat(
            output,
            gensyn_array_at(
                names,
                gensyn_string_t *,
                i
            )
        );
        if (g)
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
        gensyn_string_concat_printf(output, "%s : %f\n", gensyn_string_get_c_str(name), val);
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


















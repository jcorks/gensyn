gensyn = (function()) {
 
    return {
        'gate' : {
                
            // creates a new gate object
            'add' : function(gateType, gateName) {
                if (gateType == '' || gateName == '') throw new Error('Neither type nor name may be NULL.');

                var result = __gensyn_c_native('gate-add', gateType, gateName);

                if (result != '') throw new Error(result);

                
                
                return this.get(gateName);
            },
            
            
            // gets an object referring to a real gate
            'get' : function(gateName) {
                if (__gensyn_c_native('gate-check', gateName) != '') {
                    throw new Error(gateName + ' does not refer to a gate!');
                }

                
                return {
                    'name' : gateName,
                    
                    'remove' : function() {
                        __gensyn_c_native('gate-remove', gateName);
                    },
                    
                    
                    'summary' : function() {
                        return __gensyn_c_native('gate-summary', gateName);
                    },
                    
                    
                    'connect' : function(selfConnection, otherGateObject) {
                        var result = __gensyn_c_native('gate-connect', gateName, otherGateObject.name, selfConnection);
                        if (result != '') {
                            throw new Error(result);
                        }
                    },
                    
                    
                    'setParam' : function(paramName, value) {
                        __gensyn_c_native('gate-set-param', gateName, paramName, value);
                    },
                    

                    'getParam' : function(paramName) {
                        return Number.parse(__gensyn_c_native('gate-get-param', gateName, paramName));
                    },
                }
            },
            
            
            // returns an array of all the names of gates
            'list' : function() {
                var listRaw = __gensyn_c_native('gate-list');
                return listRaw.split('\n');
            }
        },


        
        // reduces the state of all gates into a JSON object.
        saveState : function() {
            throw new Error('Havent implemented this yet');
        },
        
        // Loads the state of gensyn from a JSON object.
        loadState : function(state) {
            throw new Error('Havent implemented this yet');
        },
        
        // returns the default output object that will receive the waveform
        getOutput : function() {
            return this.gate.get('output');
        }
        
    }
}

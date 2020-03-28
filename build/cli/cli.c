#include <gensyn/gensyn.h>
#include <stdio.h>


int main() {
    
    gensyn_t * g = gensyn_create();
    
    
    printf("GenSyn CLI\n");
    char buffer[4096];
    while(1) {
        printf("$ "); fflush(stdout);
        fgets(buffer, 4096, stdin);
        printf("\n");
        
        gensyn_send_command(g, GENSYN_STR_CAST(buffer));
        printf("\n");
    }
}

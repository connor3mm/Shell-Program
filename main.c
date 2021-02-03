#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include "display.h"
#include "parser.h"

int main(void) {

    while(1) {
        print_display_prompt();

        char input[512];
        if(fgets(input, 512, stdin) == NULL) { // End of File (CTRL+D)
            printf("Exiting...\n");
            exit(1);
        }
        // remove \n at the end of the line by replacing it with null-terminator
        input[strlen(input) - 1] = (char) 0x00;


        if(!strcmp(input, "exit")) {
            exit(1);
        }

        input_command(input);

    }
    // pass line to parser.c to return a list of commands (strings)
    // send list of commands to command.c to execute
    return 0;
}

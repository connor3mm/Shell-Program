#include <stdio.h>
#include "display.h"
#include "parser.h"

int main(void) {
    // while true get next line (string)
    // get user name to display before command


    while(1) {
        print_display_prompt();

        char input[512];
        gets(input);

        if(!strcmp(input, "exit")) {
            exit(1);
        }

        input_command(input);

    }
    // pass line to parser.c to return a list of commands (strings)
    // send list of commands to command.c to execute
    return 0;
}

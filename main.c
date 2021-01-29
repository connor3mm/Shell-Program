#include <stdio.h>
#include "display.h"

int main(void) {
    // while true get next line (string)
    // get user name to display before command
    print_display_prompt();
    // pass line to parser.c to return a list of commands (strings)
    // send list of commands to command.c to execute
    return 0;
}

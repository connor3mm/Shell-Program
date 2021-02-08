#include <stdio.h>
#include <memory.h>
#include "display.h"
#include "parser.h"

int main(void) {

    while (1) {
        print_display_prompt();

        char input[512];
        if (fgets(input, 512, stdin) == NULL) { // End of File (CTRL+D)
            printf("Exiting...\n");
            break;
        }
        // remove \n at the end of the line by replacing it with null-terminator
        input[strlen(input) - 1] = (char) 0x00;

        if (!strcmp(input, "exit")) {
            break;
        }

        input_command(input);
        putchar('\n');
    }
    return 0;
}

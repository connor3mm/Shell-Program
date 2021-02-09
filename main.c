#include <stdio.h>
#include <memory.h>
#include "display.h"
#include "parser.h"
#include <stdlib.h>
#include <unistd.h>

int main(void) {

    while (1) {
        char *currentPath = getenv("PATH"); //Gets current path so we can set it on exit

        //Set the home directory
        char *homeDirectory = getenv("HOME");
        int result = chdir(homeDirectory);

        if(result == -1) { //Changing the directory failed.
            break;
        }

        char* cwd;
        uint PATH_MAX = 4096;
        char buff[PATH_MAX + 1];

        cwd = getcwd( buff, PATH_MAX + 1 );

        if( cwd != NULL ) {
            printf( "My working directory is %s.\n", cwd );
        }

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

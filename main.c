#include <stdio.h>
#include <memory.h>
#include "display.h"
#include "parser.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

int main(void) {

    char *currentPath = getenv("PATH"); //Gets current path so we can set it on exit

    //Get the home directory
    char *homeDirectory = getenv("HOME");
    if (homeDirectory != NULL) {
        // Change to home directory
        if (chdir(homeDirectory) == -1) { //Changing the directory failed. Need to handle this somehow
            printf("Error while changing directory to $HOME: %s\n", strerror(errno));
        }
    }

    while (1) {
        print_display_prompt();

        char input[512];
        if (fgets(input, 512, stdin) == NULL) { // End of File (CTRL+D)
            printf("Exiting...\n");
            break;
        }
        // remove \n at the end of the line by replacing it with null-terminator
        input[strlen(input) - 1] = (char) 0x00;

        char *tokens[51];
        // treat all delimiters as command line argument separators according to the spec
        char *pChr = strtok(input, " \t|><&;");
        if (pChr == NULL) { // not even one token (empty command line)
            return 0;
        }
        int index = 0;
        while (pChr != NULL) {
            if (index >= 50) {
                printf("Argument limit exceeded");
                break;
            }
            tokens[index] = pChr;
            pChr = strtok(NULL, " \t|><&;");
            index++;
        }
        // add null terminator as required by execvp
        tokens[index] = NULL;

        if (!strcmp(tokens[0], "exit")) {
            break;
        } else if (!strcmp(tokens[0], "getpath")) {
            // get the PATH
            continue;
        } else if (!strcmp(tokens[0], "getpath")) {
            // set the PATH
            continue;
        } else {
            int pid = fork();
            if (pid < 0) {
                printf("fork() failed\n");
                return 0;
            } else if (pid == 0) { // child process
                execvp(tokens[0], tokens);
                // exec functions do not return if successful, this code is reached only due to errors
                printf("Error: %s\n", strerror(errno));
                exit(1);
            } else { // parent process
                int state;
                waitpid(pid, &state, 0);
            }
        }
    }
    setenv("PATH", currentPath, 1);
    return 0;
}

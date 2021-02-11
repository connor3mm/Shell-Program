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
    // this should be 0 on successful run, 1 on error
    int statusCode = 0;

    char *currentPath = getenv("PATH"); // Gets current path so we can set it on exit

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
            continue;
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

        // check for built-in commands before forking
        if (!strcmp(tokens[0], "exit")) {
            break;
        } else if (!strcmp(tokens[0], "getpath")) {
            printf("%s\n", getenv("PATH")); 
        } else if (!strcmp(tokens[0], "setpath")) {
            if (tokens[1] == NULL)
            {
                printf("Error, setpath requires an argument.\n");
            } else
            {
                setenv("PATH", tokens[1], 1);
            }
            
            continue;
        } else if (!strcmp(tokens[0], "cd")) {

            if (tokens[1] == NULL)
            {
                chdir(getenv("HOME"));
            } else
            {
                if (chdir(tokens[1]) == -1)
                {
                    printf("Error: %s\n", strerror(errno));
                }    

            }
        }
        else {
            int pid = fork();
            if (pid < 0) {
                printf("fork() failed\n");
                statusCode = 1;
                break;
            } else if (pid == 0) { // child process
                execvp(tokens[0], tokens);
                // exec functions do not return if successful, this code is reached only due to errors
                printf("Error: %s\n", strerror(errno));
                statusCode = 1;
                break;
            } else { // parent process
                int state;
                waitpid(pid, &state, 0);
            }
        }
    }
    setenv("PATH", currentPath, 1);
    return statusCode;
}

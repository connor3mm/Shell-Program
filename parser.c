#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

/*
* This function will allow the user to input into a command to the terminal
*/
void input_command(char *input) {
    char *tokens[51];
    // treat all delimiters as command line argument separators according to the spec
    char *pChr = strtok(input, " \t|><&;");
    int index = 0;
    while (pChr != NULL) {
        if (index >= 50) {
            printf("Argument limit exceeded");
            break;
        }
        tokens[index] = pChr;
        printf("%s ", tokens[index]);
        pChr = strtok(NULL, " \t|><&;");
        index++;
    }
    // add null terminator as required by execve
    tokens[index] = NULL;

    int pid = fork();
    if (pid < 0) {
        printf("fork() failed");
        exit(1);
    } else if (pid == 0) { // child process
        char* env = {NULL}; //todo: pass PATH
        execve(tokens[0], tokens, &env);
        // exec functions do not return if successful, this code is reached only due to errors
        printf("Error: %s\n", strerror(errno));
        exit(1);
    } else { // parent process
        int state;
        waitpid(pid, &state, 0);
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

/*
* This function will allow the user to input into a command to the terminal
*/
void input_command(char *input) {

    // using delimiters to slip the input

    char *pChr = strtok(input, "\t|><&;");

    char *tokens[50];
    int index = 0;

    while (pChr != NULL) {

        tokens[index] = pChr;
        printf("%s ", tokens[index]);
        index++;

        int pid = fork();
        if (pid == -1) {
            exit(1);
        } else if (pid == 0) {
            // todo: use strtok_r to split command line arguments
            char *arg_list = {tokens[index], NULL};
            execv(tokens[index], &arg_list);
            printf("The current process is (%u)\n", getpid());
            exit(1);
        } else {
            printf("The current non-0 process is (%u)\n", getpid());
            int state;
            // todo: redirect stderr and stdout
            waitpid(pid, &state, 0);
            pChr = strtok(NULL, "\t|><&;");
        }
    }
}
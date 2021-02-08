#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define _GNU_SOURCE

/*
* This function will allow the user to input into a command to the terminal
*/
void input_command(char *input) {
    char *tokens[51];
    // treat all delimiters as command line argument separators according to the spec
    char *pChr = strtok(input, " \t|><&;");
    if (pChr == NULL) { // not even one token (empty command line)
        return;
    }
    int index = 0;
    while (pChr != NULL) {
        if (index >= 50) {
            printf("Argument limit exceeded");
            break;
        }
        tokens[index] = pChr;
        //printf("%s ", tokens[index]);
        pChr = strtok(NULL, " \t|><&;");
        index++;
    }
    // add null terminator as required by execve
    tokens[index] = NULL;

    int pid = fork();
    if (pid < 0) {
        printf("fork() failed\n");
        return;

    } else if (pid == 0) { // child process
        char* path_value = getenv("PATH");
        // copy the path into PATH=value format with the path_key_value buffer
        int keyLength = strlen("PATH=");
        int valueLength = strlen(path_value);
        char* path_key_value = malloc(keyLength + valueLength + 1);
        strncpy(path_key_value, "PATH=", keyLength);
        strncpy(path_key_value + keyLength, path_value, valueLength);

//        printf("%s\n", path_key_value);
        char *env = {path_key_value, NULL};
        execvpe(tokens[0], tokens, &env);
        // exec functions do not return if successful, this code is reached only due to errors
        printf("Error: %s\n", strerror(errno));
        exit(1);
    } else { // parent process
        int state;
        waitpid(pid, &state, 0);
    }
}

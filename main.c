#include <stdio.h>
#include <memory.h>
#include "display.h"
#include "parser.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

char *history[20];
int currentHistoryIndex = 0;

void saveHistory();
void loadHistory();

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

    int historySize = 20;


    loadHistory();

    while (1) {
        print_display_prompt();

        char input[512];
        if (fgets(input, 512, stdin) == NULL) { // End of File (CTRL+D)
            printf("Exiting...\n");
            break;
        }
        // remove \n at the end of the line by replacing it with null-terminator
        input[strlen(input) - 1] = (char) 0x00;

        // Check if it's a history command
        int historyNumber = 0;
        if (input[0] == '!' && strlen(input) > 1) {
            if(input[1] == '!' && strlen(input) > 2){
                printf("Invalid input\n");
                continue;
            }
            else if(input[1] == '!') {
                historyNumber = currentHistoryIndex;
            } else if (input[1] == '-' && strlen(input) < 5)
            {
                if (strlen(input) > 3 && (input[2] - '0' > 0) && ((input[2] - '0') < currentHistoryIndex -1 )) {
                    printf("Invalid input\n");
                    continue;
                }

                int num = input[2] - '0';

                historyNumber = currentHistoryIndex - num;
            }

            else {
                // convert string after ! to integer
                historyNumber = (int) strtol(input + 1, NULL, 10);
            }

            if (errno != 0) {
                printf("Error: %s\n", strerror(errno));
                continue;
            } else if (historyNumber < 1 || historyNumber > 20) {

                printf("Invalid history number, history entries range from 1 to 20\n");
                continue;
                
            } else if (historyNumber > currentHistoryIndex) {
                printf("History command does not exist for this invocation\n");
                continue;
            }
        }

        char *tokens[51];
        // treat all delimiters as command line argument separators according to the spec
        char *pChr;
        // checking for a history command
        if (historyNumber != 0) {
            // tokenize from history entry
            pChr = history[historyNumber - 1];
        } else {
            // add command line to history
            if (currentHistoryIndex == historySize)
            {
                currentHistoryIndex = 0;
            }

            history[currentHistoryIndex] = strdup(input);
            currentHistoryIndex++;
            pChr = strtok(input, " \t|><&;");
        }

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
            if (tokens[1] != NULL) {
                printf("Error, getpath does not take any arguments.\n");
                continue;
            }
            printf("%s\n", getenv("PATH"));
        }else if (!strcmp(tokens[0], "history")) {
            int count = 0;
            while(history[count + 1 ] != NULL) {
                printf("%d %s\n", count + 1, history[count]);
                count++;
            }
            continue;
        }
        else if (!strcmp(tokens[0], "setpath")) {
            if (tokens[2] != NULL) {
                printf("Error, setpath can only take one argument.\n");
                continue;
            } else if (tokens[1] == NULL) {
                printf("Error, setpath requires an argument.\n");
            } else {
                setenv("PATH", tokens[1], 1);
            }

            continue;
        } else if (!strcmp(tokens[0], "cd")) {
            if (tokens[1] == NULL) {
                chdir(getenv("HOME"));
            } else if (tokens[2] == NULL) {
                if (chdir(tokens[1]) == -1) {
                    printf("Error: %s %s\n", tokens[1], strerror(errno));
                }
            } else if (tokens[2] != NULL) {
                printf("Error, cd only takes one argument\n");
                continue;
            }
        } else {
            int pid = fork();
            if (pid < 0) {
                printf("fork() failed\n");
                statusCode = 1;
                break;
            } else if (pid == 0) { // child process
                execvp(tokens[0], tokens);
                // exec functions do not return if successful, this code is reached only due to errors
                printf("Error: %s %s\n", tokens[0], strerror(errno));
                statusCode = 1;
                break;
            } else { // parent process
                int state;
                waitpid(pid, &state, 0);
            }
        }
    }
    saveHistory();
    setenv("PATH", currentPath, 1);
    return statusCode;
}

void saveHistory(){
    FILE *p;
    p = fopen(".hist_list", "w");

    for(int i = 0; i < 20; i++) {
        if(history[i] != NULL){
            fputs(history[i], p);
            fprintf(p, "\n");
        } else {
            break;
        }
    }
    fclose(p);

}

void loadHistory(){
    FILE *pFile;
    pFile = fopen(".hist_list", "r");
    if (pFile == NULL) {
        printf("Error!! File does not exist!! \n");
        return;
    }
    int count = 0;
    char buffer[1000];
    while(fgets(buffer, 1000, pFile) != NULL) {
        size_t length = strlen(buffer);
        if (length > 0 && buffer[length-1] == '\n') {
            buffer[--length] = '\0';
        }

        char *string = malloc(sizeof(buffer));

        strcpy(string, buffer);
        history[count] = string;
        count++;

    }
    currentHistoryIndex = count;
    fclose(pFile);

}


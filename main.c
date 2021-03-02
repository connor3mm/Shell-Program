#include <stdio.h>
#include <memory.h>
#include "display.h"
#include "parser.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#define HISTORY_LIMIT 5

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
    char *history[HISTORY_LIMIT];
    int historySize = 0;
    int currentHistoryIndex = 0; // back of queue
    int oldestHistoryIndex = 0; // front of queue

    while (1) {
        print_display_prompt();

        char input[512];
        if (fgets(input, 512, stdin) == NULL) { // End of File (CTRL+D)
            printf("Exiting...\n");
            break;
        }
        // remove \n at the end of the line by replacing it with null-terminator
        input[strlen(input) - 1] = (char) 0x00;

        // history number selected by the user - can be positive or negative
        int selectedHistoryNumber = 0;
        // whether this is a history command
        int isHistory = 0;
        // check whether this is a history command starting with !
        if (input[0] == '!') {

            if(strlen(input) == 1) {
                printf("! requires a numeric argument\n");
                    continue;
            }

            // !! - invoke last command
            if(input[1] == '!') {
                if(strlen(input) > 2){
                    printf("Invalid input after !!\n");
                    continue;
                } else {
                    // same as saying !-1 to get the last one
                    strcpy(input, "!-1");
                }
            }
            // !{number} - invoke command at index
            // get the number after ! first
            selectedHistoryNumber = (int) strtol(input + 1, NULL, 10);
            // check whether number parsing was successful
            if (errno != 0) {
                printf("Error: %s\n", strerror(errno));
                continue;
            }
            // do not allow 0 or values bigger than current history size
            if(selectedHistoryNumber == 0 || selectedHistoryNumber > historySize || selectedHistoryNumber < -historySize ) {
                printf("Invalid history index\n");
                continue;
            }
            isHistory = 1;
            // check if number is positive and select index starting from oldestHistoryIndex
            if(selectedHistoryNumber > 0) {
                selectedHistoryNumber--;
                selectedHistoryNumber = (oldestHistoryIndex + selectedHistoryNumber) % (HISTORY_LIMIT);
            }
            // in this case selectedHistoryNumber is definitely negative, select index in reverse starting from currentHistoryIndex
            else {
                int offsetFromLatest = currentHistoryIndex + selectedHistoryNumber;
                if(offsetFromLatest < 0) {
                    // subtract remaining offset from the end of array
                    selectedHistoryNumber = HISTORY_LIMIT + offsetFromLatest;
                } else {
                    selectedHistoryNumber = offsetFromLatest;
                }
            }
        }

        char *tokens[51];
        // treat all delimiters as command line argument separators according to the spec
        char *pChr;
        // checking for a history command
        if (isHistory) {
            // tokenize from history entry
            // the strtok function modifies any input string by cutting out the tokens, so use input as temporary variable for strtok
            strcpy(input, history[selectedHistoryNumber]);
            pChr = strtok(input, " \t|><&;");
        } else {
            // add command line to history
            history[currentHistoryIndex] = strdup(input);
            pChr = strtok(input, " \t|><&;");

            currentHistoryIndex++;
            // wrap around next item index
            if (currentHistoryIndex == HISTORY_LIMIT) {
                currentHistoryIndex = 0;
            }
            // history is full and has wrapped around - move oldest item index so we don't get the last one
            if(currentHistoryIndex > oldestHistoryIndex && historySize == HISTORY_LIMIT) {
                oldestHistoryIndex++;
            }
            if(oldestHistoryIndex == HISTORY_LIMIT) {
                oldestHistoryIndex = 0;
            }
            // increase size
            if(historySize != HISTORY_LIMIT) {
                historySize++;
            }
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
        } else if (!strcmp(tokens[0], "setpath")) {
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
    setenv("PATH", currentPath, 1);
    return statusCode;
}

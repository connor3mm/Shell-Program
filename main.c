#include <stdio.h>
#include <memory.h>
#include "display.h"
#include "parser.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>


#define HISTORY_LIMIT 20
char *history[HISTORY_LIMIT];
int currentHistorySize = 0;
int currentHistoryIndex = 0;
int oldestHistoryIndex = 0;
const char *aliasCommands[10][2];

void saveHistory();
void loadHistory();
void addAliases(char *name, char *command);

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

        // boolean - whether this is a history command
        int isHistoryCommand = 0;

        int historyNumber = 0;
        // Check if it's a history command
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
            char* endPointer = NULL;
            historyNumber = (int) strtol(input + 1, &endPointer, 10);
            // check whether number parsing was successful
            if((*endPointer) != '\0') {
                printf("Invalid history input\n");
                continue;
            }
            if (errno != 0) {
                printf("Error: %s\n", strerror(errno));
                continue;
            }
            // do not allow 0 or values bigger than current history size
            if(historyNumber == 0 || historyNumber > currentHistorySize || historyNumber < -currentHistorySize ) {
                printf("Invalid history index\n");
                continue;
            }
            isHistoryCommand = 1;
            // check if number is positive
            if(historyNumber > 0) {
                // turn it into an index
                historyNumber--;
                // start from the oldestHistoryIndex rather than 0
                historyNumber = (oldestHistoryIndex + historyNumber) % (HISTORY_LIMIT);
            }
                // in this case historyNumber is definitely negative
            else {
                // subtract from current index
                int offsetFromLatest = currentHistoryIndex + historyNumber;
                if(offsetFromLatest < 0) {
                    // index has gone negative, wrap around from the end of the array
                    historyNumber = HISTORY_LIMIT + offsetFromLatest;
                } else {
                    historyNumber = offsetFromLatest;
                }
            }
        }

        char *tokens[51];
        // treat all delimiters as command line argument separators according to the spec
        char *pChr;
        // checking for a history command
        if (isHistoryCommand) {
            // tokenize from history entry
            strcpy(input, history[historyNumber]);
        } else {
            //check if the command is history
            if(!strcmp(input, "history")  && history[0] == NULL) {
                printf("There is not history commands to display.\n");
                history[currentHistoryIndex] = strdup(input);
                continue;
            }

            // add command line to history
            history[currentHistoryIndex] = strdup(input);

            currentHistoryIndex++;
            // wrap around next item index
            if (currentHistoryIndex == HISTORY_LIMIT) {
                currentHistoryIndex = 0;
            }
            // history is full and has wrapped around - move oldest item index so we don't get the last one when typing !1
            if(currentHistoryIndex > oldestHistoryIndex && currentHistorySize == HISTORY_LIMIT) {
                oldestHistoryIndex++;
            }
            if(oldestHistoryIndex == HISTORY_LIMIT) {
                oldestHistoryIndex = 0;
            }
            // increase size
            if(currentHistorySize != HISTORY_LIMIT) {
                currentHistorySize++;
            }

        }
        pChr = strtok(input, " \t|><&;");

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
            if(tokens[1] != NULL) {
                printf("Error, history can only take one argument.\n");
                continue;
            }

            for(int i=0; i<currentHistorySize; i++) {
                printf("%d %s\n", i+1, history[(oldestHistoryIndex + i) % HISTORY_LIMIT]);
            }
        }





        else if (!strcmp(tokens[0], "alias")) {
            if(tokens[3] != NULL) {
                printf("Error, alias can only take two argument.\n");
                continue;
            }
            addAliases( tokens[1],tokens[2]);






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
    for(int i=0; i<currentHistorySize; i++) {
        fputs(history[(oldestHistoryIndex + i) % HISTORY_LIMIT], p);
        fprintf(p, "\n");
    }
    fclose(p);

}

void loadHistory(){
    FILE *pFile;
    pFile = fopen(".hist_list", "r");
    if (pFile == NULL) {
        printf("History file not found.\n");
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
    currentHistorySize = count;
    currentHistoryIndex = count % HISTORY_LIMIT;
    fclose(pFile);

}

void addAliases(char *name, char *command){
    int count = 0;
    for (int i = 0; i < 10; ++i) {
        if(aliasCommands[i][0] != NULL) {
            count++;
        }
    }

    if(count == 10) {
        printf("Error, no more aliases available.\n");
        return;
    }

    int index = 0;
    while(aliasCommands[index][0] != NULL) {
        if(strcmp(aliasCommands[index][0], name) == 0) {
            printf("This name already exists and cannot be used again.\n");
            return;
        }
        index++;
    }

    aliasCommands[index][0] = strdup(name);
    aliasCommands[index][1] = strdup(command);
}


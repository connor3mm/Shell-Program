#include <stdio.h>
#include <memory.h>
#include "display.h"
#include "parser.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#include "alias.c"
#include "history.h"


/*
 * function declaration 
 */
void run();

int tokenizeInput(char *input, char **tokens, char *pChr);

void getPath(char *pString[51]);

void setPath(char *pString[51]);

void setCd(char *pString[51]);

int isCommandEmpty(char *string);


/*
 * Main programme
 */
int main(void) {

    char *currentPath = getenv("PATH"); // Gets current path so we can set it on exit
    char *homeDirectory = getenv("HOME"); //Get the home directory

    //changeToHomeDirectory(homeDirectory);
    if (homeDirectory != NULL) {
        // Change to home directory
        if (chdir(homeDirectory) == -1) { //Changing the directory failed. Need to handle this somehow
            printf("Error while changing directory to $HOME: %s\n", strerror(errno));
        }
    }

    loadHistory();
    loadAliases();
    run();
    if (homeDirectory != NULL) {
        // Change to home directory
        if (chdir(homeDirectory) == -1) { //Changing the directory failed. Need to handle this somehow
            printf("Error while changing directory to $HOME: %s\n", strerror(errno));
        }
    }
    saveHistory();
    saveAliases();

    setenv("PATH", currentPath, 1);
    return 0;

}


//Running of the program
void run() {

    while (1) {
        errno = 0;
        print_display_prompt();

        char input[512];

        if (fgets(input, 512, stdin) == NULL) { // End of File (CTRL+D)
            printf("Exiting...\n");
            break;
        }

        // remove \n at the end of the line by replacing it with null-terminator
        input[strlen(input) - 1] = (char) 0x00;

        if (isCommandEmpty(input)) {
            continue;
        }

        // boolean - whether this is a history command
        int isHistoryCommand = 0;

        int historyNumber = 0;

        // Check if it's a history command
        if (input[0] == '!') {

            if (strlen(input) == 1) {
                printf("! requires a numeric argument\n");
                continue;
            }

            // !! - invoke last command
            if (input[1] == '!') {
                if (strlen(input) > 2) {
                    printf("Invalid input after !!\n");
                    continue;
                } else {
                    // same as saying !-1 to get the last one
                    strcpy(input, "!-1");
                }
            }

            // !{number} - invoke command at index
            // get the number after ! first
            char *endPointer = NULL;
            historyNumber = (int) strtol(input + 1, &endPointer, 10);
            // check whether number parsing was successful
            if ((*endPointer) != '\0') {
                printf("Invalid history input\n");
                continue;
            }

            if (errno != 0) {
                printf("Error: %s\n", strerror(errno));
                continue;
            }

            // do not allow 0 or values bigger than current history size
            if (historyNumber == 0 || historyNumber > currentHistorySize || historyNumber < -currentHistorySize) {
                printf("Invalid history index\n");
                continue;
            }

            isHistoryCommand = 1;

            // check if number is positive
            if (historyNumber > 0) {
                // turn it into an index
                historyNumber--;
                // start from the oldestHistoryIndex rather than 0
                historyNumber = (oldestHistoryIndex + historyNumber) % (HISTORY_LIMIT);
            }

                // in this case historyNumber is definitely negative
            else {
                // subtract from current index
                int offsetFromLatest = currentHistoryIndex + historyNumber;

                if (offsetFromLatest < 0) {
                    // index has gone negative, wrap around from the end of the array
                    historyNumber = HISTORY_LIMIT + offsetFromLatest;
                } else {
                    historyNumber = offsetFromLatest;
                }
            }
        }


        char *tokens[51];
        // treat all delimiters as command line argument separators according to the spec
        char *pChr = NULL;
        // checking for a history command
        if (isHistoryCommand) {
            // tokenize from history entry
            strcpy(input, history[historyNumber]);
        } else {
            //check if the command is history
            if (!strcmp(input, "history") && history[0] == NULL) {
                printf("There is not history commands to display.\n");
                history[currentHistoryIndex] = strdup(input);
                //continue;
            }

            // add command line to history
            history[currentHistoryIndex] = strdup(input);

            currentHistoryIndex++;
            // wrap around next item index
            if (currentHistoryIndex == HISTORY_LIMIT) {
                currentHistoryIndex = 0;
            }
            // history is full and has wrapped around - move oldest item index so we don't get the last one when typing !1
            if (currentHistoryIndex > oldestHistoryIndex && currentHistorySize == HISTORY_LIMIT) {
                oldestHistoryIndex++;
            }
            if (oldestHistoryIndex == HISTORY_LIMIT) {
                oldestHistoryIndex = 0;
            }
            // increase size
            if (currentHistorySize != HISTORY_LIMIT) {
                currentHistorySize++;
            }

        }


        //splitting input with tokens - don't go on if it fails
        if (!tokenizeInput(input, tokens, pChr)) {
            continue;
        }

        // check for built-in commands before forking
        //Check for exit
        if (!strcmp(tokens[0], "exit")) {
            break;


            //Checking getpath
        } else if (!strcmp(tokens[0], "getpath")) {
            getPath(tokens);
            continue;

            //Checking History
        } else if (!strcmp(tokens[0], "history")) {
            getHistory(tokens);
            continue;

            //Checking for Alias
        } else if (!strcmp(tokens[0], "alias")) {
            if (tokens[1] == NULL) {
                printAlias();
                continue;
            } else if (tokens[1] != NULL && tokens[2] == NULL) {
                printf("Error, alias needs at least one argument.\n");
                continue;
            }
            addAliases(tokens);
            continue;

            //Removing alias
        } else if (!strcmp(tokens[0], "unalias")) {
            if (tokens[2] != NULL) {
                printf("Error, alias can only take one argument.\n");
                continue;
            }
            unAlias(tokens[1]);
            continue;
        }

        replaceAliases(tokens);


        //Sets the path
        if (!strcmp(tokens[0], "setpath")) {
            setPath(tokens);
            continue;


            //Checking for cd command
        } else if (!strcmp(tokens[0], "cd")) {
            setCd(tokens);


            //Activating forking
        } else {
            int pid = fork();
            if (pid < 0) {
                printf("fork() failed\n");
                break;
            } else if (pid == 0) { // child process
                execvp(tokens[0], tokens);
                // exec functions do not return if successful, this code is reached only due to errors
                printf("Error: %s %s\n", tokens[0], strerror(errno));
                break;
            } else { // parent process
                int state;
                waitpid(pid, &state, 0);
            }
        }

    }
}



/*
 * Start of functions
 */


/*
 * Changing to the home directory
 */
void changeToHomeDirectory(const char *homeDirectory) {
    if (homeDirectory != NULL) {
        // Change to home directory
        if (chdir(homeDirectory) == -1) { //Changing the directory failed. Need to handle this somehow
            printf("Error while changing directory to $HOME: %s\n", strerror(errno));
        }
    }
}


/**
 * Tokenizing the input the user makes
 * @param input
 * @param tokens
 * @param pChr
 * @return Return 0 on failure, 1 on success
 */
int tokenizeInput(char *input, char **tokens, char *pChr) {
    pChr = strtok(input, " \t|><&;");
    if (pChr == NULL) { // not even one token (empty command line)
        return 0;
    }
    int index = 0;
    while (pChr != NULL) {
        if (index >= 50) {
            printf("Argument limit exceeded");
            return 0;
        }
        tokens[index] = pChr;
        pChr = strtok(NULL, " \t|><&;");
        index++;
    }
    // add null terminator as required by execvp
    tokens[index] = NULL;
    return 1;
}


/*
 * Definition for the ''cd' command
 */
void setCd(char *tokens[51]) {
    if (tokens[1] == NULL) {
        chdir(getenv("HOME"));
        return;
    } else if (tokens[2] == NULL) {
        if (chdir(tokens[1]) == -1) {
            printf("Error: %s %s\n", tokens[1], strerror(errno));
            return;
        }
    } else if (tokens[2] != NULL) {
        printf("Error, cd only takes one argument\n");
        return;
    }
}


/*
 * Definition for the history command
 */
void getHistory(char *tokens[51]) {
    if (tokens[1] != NULL) {
        printf("Error, history can only take one argument.\n");
        return;
    }
    for (int i = 0; i < currentHistorySize; i++) {
        printf("%d %s\n", i + 1, history[(oldestHistoryIndex + i) % HISTORY_LIMIT]);
    }
}


/*
 * Definition for the setpath command
 */
void setPath(char *tokens[51]) {
    if (tokens[2] != NULL) {
        printf("Error, setpath can only take one argument.\n");
        return;
    } else if (tokens[1] == NULL) {
        printf("Error, setpath requires an argument.\n");
        return;
    } else {
        setenv("PATH", tokens[1], 1);
        return;
    }
}


/*
 * Definition for getpath command
 */
void getPath(char *tokens[51]) {
    if (tokens[1] != NULL) {
        printf("Error, getpath does not take any arguments.\n");
        return;
    }
    printf("%s\n", getenv("PATH"));
}


/*
 * Saving history
 */
void saveHistory() {
    FILE *p;
    p = fopen(".hist_list", "w");
    for (int i = 0; i < currentHistorySize; i++) {
        fputs(history[(oldestHistoryIndex + i) % HISTORY_LIMIT], p);
        fprintf(p, "\n");
    }
    fclose(p);
}


/*
 * Loading history from file to history array
 */
void loadHistory() {
    FILE *pFile;

    pFile = fopen(".hist_list", "r");
    if (pFile == NULL) {
        printf("History file not found.\n");
        return;
    }

    int count = 0;
    char buffer[1000];

    while (fgets(buffer, 1000, pFile) != NULL) {
        size_t length = strlen(buffer);
        if (length > 0 && buffer[length - 1] == '\n') {
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


/**
 * Checks if the user input is empty
 * @param string
 * @return number of separators
 */
int isCommandEmpty(char *string) {
    char *separators = " \t|><&;";
    size_t numSeparators = 0;
    // for every char in the string
    for (int i = 0; i < strlen(string); i++) {
        // check if it is in the separators
        for (int t = 0; t < strlen(separators); t++) {
            if (string[i] == separators[t]) {
                numSeparators++;
                break;
            }
        }
    }
    // whether this string was just separators
    return numSeparators == strlen(string);
}

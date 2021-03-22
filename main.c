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

typedef struct Alias {
    char* name;
    char* commandTokens[50];
    int numCommandTokens;
    struct Alias* linkedCommands[10];
    int numLinkedCommands;
    // only used for detecting loops
    int visited;
} Alias;
Alias* aliasList[10];

/*
 * function declaration 
 */
void saveHistory();

void loadHistory();

void addAliases(char **tokens);

void unAlias(char *name);

void printAlias();

void replaceAliases(char **tokens);

void saveAliases();

void loadAliases();

void rebuildAliasLinks();

void linkAliases();

int checkAliasLoop(Alias* current, int stack[]);

void run();

void changeToHomeDirectory(const char *homeDirectory);

int tokenizeInput(char *input, char **tokens, char *pChr);

void getPath(char *pString[51]);

void setPath(char *pString[51]);

void getHistory(char *pString[51]);

void setCd(char *pString[51]);


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
        if( !tokenizeInput(input, tokens, pChr) ) {
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


/*
 * Tokenizing the input the user makes
 * Return 0 on failure, 1 on success
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

/*
* Adding alias
*/
void addAliases(char **tokens) {
    char *name = tokens[1];

    // Check if there's a free slot in our array of 10 aliases
    int freeSlot = 0;
    for (int i = 0; i < 10; ++i) {
        if (aliasList[i] != NULL) {
            freeSlot++;
        } else {
            break; // we've found an empty slot in the aliasCommands array!
        }
    }

    if (freeSlot == 10) {
        printf("Error, no more aliases available.\n");
        return;
    }

    // Check for duplicate names
    for(int i=0; i < 10; i++) {
        if(aliasList[i] != NULL && !strcmp(aliasList[i]->name, name) ) {
            printf("Error: This name already exists and cannot be used again.\n");
            return;
        }
    }

    // Create and add alias, using a persistent pointer with malloc.
    Alias* alias = malloc(sizeof(Alias));
    // Set the name
    alias->name = strdup(name);
    alias->numCommandTokens = 0;

    int nextTokenIndex = 0;
    // copy command tokens starting from 2, since we have 'alias' at 0 and the name at 1
    while (tokens[nextTokenIndex + 2] != NULL) {
        if(!strcmp(name, tokens[nextTokenIndex + 2])) {
            printf("Can't add alias, circular definition detected\n");
            return;
        }
        alias->commandTokens[nextTokenIndex] = strdup(tokens[nextTokenIndex + 2]);
        alias->numCommandTokens++;
        nextTokenIndex++;
    }

    alias->numLinkedCommands = 0;
    alias->visited = 0;
    aliasList[freeSlot] = alias;

    rebuildAliasLinks();
    // detect loops
    int loop = 0;
    int stack[100];
    for(int i=0; i<10; i++) {
        // reset flags
        for(int i=0; i<10; i++) {
            if(aliasList[i] != NULL) {
                aliasList[i]->visited = 0;
            }
        }
        memset(stack, 0x00, 100);
        if(aliasList[i] != NULL && checkAliasLoop(aliasList[i], stack))
            loop = 1;
    }

    // If the definition we've added is causing a loop, remove it
    if(loop) {
        printf("Can't add alias, circular definition detected\n");
        free(aliasList[freeSlot]);
        aliasList[freeSlot] = NULL;
        rebuildAliasLinks();
    }
}


/*
* Removing alias
*/
void unAlias(char *name) {
    int removed = 0;
    for(int i=0; i<10; i++) {
        if( aliasList[i] != NULL && !strcmp(aliasList[i]->name, name) ) {
            // free Alias pointer and set it to null
            free(aliasList[i]);
            aliasList[i] = NULL;
            // set flag for the success message
            removed = 1;
            break;
        }
    }
    if(!removed) {
        printf("The alias you entered does not exist.\n");
    } else {
        rebuildAliasLinks();
        printf("Alias successfully removed.\n");
    }
}


/*
 * Print list of aliases
 */
void printAlias() {
    // How many aliases we've stored.
    int aliasesFound = 0;
    // Current alias index
    int aliasIndex = 0;
    while (aliasIndex < 10) {
        if (aliasList[aliasIndex] != NULL) {
            aliasesFound++;
            printf("Name: %s - Command: ", aliasList[aliasIndex]->name );
            int aliasTokenIndex = 0;
            for(int i=0; i<aliasList[aliasIndex]->numCommandTokens; i++) {
                printf("%s ", aliasList[aliasIndex]->commandTokens[i]);
                aliasTokenIndex++;
            }
            printf("\n");
        }
        aliasIndex++;
    }

    if (aliasesFound == 0) {
        printf("There are no aliases set.\n");
    }
}

int getIndexFromAlias(Alias* alias) {
    for(int i=0; i<10; i++) {
        if(aliasList[i] != NULL && !strcmp(alias->name, aliasList[i]->name))
            return i;
    }
    return -1;
}

/*
 * For each alias, go over the other aliases and build a list of any aliases it is using.
 * This makes it easier to check for loops later.
 */
void linkAliases() {
    for(int firstAlias=0; firstAlias<10; firstAlias++) {
        // skip over empty slots
    	if(aliasList[firstAlias] == NULL)
    	    continue;
        // skip over empty slots and the alias we're comparing
    	for(int secondAlias=0; secondAlias<10; secondAlias++) {
            if(aliasList[secondAlias] == NULL || firstAlias == secondAlias)
                continue;
            for(int token=0;token<aliasList[secondAlias]->numCommandTokens;token++) {
                if( !strcmp(aliasList[firstAlias]->name, aliasList[secondAlias]->commandTokens[token]) ) {
                    int numLinkedCommands = aliasList[secondAlias]->numLinkedCommands;
                    aliasList[secondAlias]->linkedCommands[numLinkedCommands] = aliasList[firstAlias];
                    aliasList[secondAlias]->numLinkedCommands++; 
                }
            }
        }
    }
}

/*
 * Recursive function to check for loops - check every linked alias, and mark every node as visited
 * If we meet a node we've already visited that's in our recursion array that means there's a loop. 
 * The stack is necessary, otherwise it would only be good for an undirected graph
 */
int checkAliasLoop(Alias* current, int stack[]) {
    if(!current->visited) 
    { 
        // Mark the current node as visited
        current->visited = 1;
        // Add index of node to list
        stack[getIndexFromAlias(current)] = 1;

        for(int i=0; i<current->numLinkedCommands; i++) {
            int linkedCommandIndex = getIndexFromAlias(current->linkedCommands[i]);
            // if this node's subtree contains a loop return true
            if(!aliasList[linkedCommandIndex]->visited && checkAliasLoop(current->linkedCommands[i], stack) )
                return 1;
            // same if we've already checked out this node
            else if(stack[linkedCommandIndex])
                return 1;
        }
    }
    stack[getIndexFromAlias(current)] = 0;
    return 0;
}

/*
 * Reset links for every alias, then call linkAliases to rebuild them.
 * This ensures there are no empty slots in the linkedCommands array if we remove an alias.
 */
void rebuildAliasLinks() {
    for(int i=0; i<10; i++) {
        if(aliasList[i] != NULL) {
            aliasList[i]->numLinkedCommands = 0;
        }
    }
    linkAliases();
}


/*
 * Keep swapping any alias with their command until there are no more substitutions
 */
void replaceAliases(char **tokens) {
    int tokenIndex = 0;
    while (tokens[tokenIndex] != NULL) {
        int replacements = 0;
        // check whether this token has an alias equivalent
        for (int aliasIndex = 0; aliasIndex < 10; aliasIndex++) {
            if (aliasList[aliasIndex] == NULL) {
                continue;
            }
            if (!strcmp(tokens[tokenIndex], aliasList[aliasIndex]->name)) {
                replacements++;
                // get number of tokens from alias
                int aliasTokens = aliasList[aliasIndex]->numCommandTokens;
                // get number of tokens in input after the alias
                int tokensLeft = 0;
                while (tokens[tokenIndex + tokensLeft + 1] != NULL)
                    tokensLeft++;
                // move remaining tokens after alias into a temp array so we don't lose them
                char **tokensToShift = malloc(tokensLeft);
                for (int i = 0; i < tokensLeft; i++) {
                    tokensToShift[i] = tokens[tokenIndex + 1 + i];
                }
                // move alias command tokens into input array, starting from the alias
                for (int i = 0; i < aliasTokens; i++) {
                    tokens[tokenIndex + i] = aliasList[aliasIndex]->commandTokens[i];
                }
                // move tokens from temp array back to our main tokens array, starting after the alias command
                for (int i = 0; i < tokensLeft; i++) {
                    tokens[tokenIndex + aliasTokens + i] = tokensToShift[i];
                }
                tokens[tokenIndex + aliasTokens + tokensLeft] = NULL;
                free(tokensToShift);

            }
        }
        // no replacements were performed, check next token
        if(replacements == 0)
            tokenIndex++;
    }
}


/*
 * save aliases into file
 */
void saveAliases() {
    FILE *a;
    a = fopen(".aliases", "w");

    char aliasLine[512];

    for (int aliasIndex = 0; aliasIndex < 10; aliasIndex++) {
        // Skip over empty slots in the array
        if (aliasList[aliasIndex] == NULL) {
            continue;
        }
        // Copy name into output line
        strcat(aliasLine, aliasList[aliasIndex]->name);
        strcat(aliasLine, " ");
        // Copy every token into output line, add a space after each one
        for(int i=0; i<aliasList[aliasIndex]->numCommandTokens; i++) {
            strcat(aliasLine, aliasList[aliasIndex]->commandTokens[i]);
            strcat(aliasLine, " ");
        }
        // replace the last space with newline
        aliasLine[strlen(aliasLine) - 1] = '\n';
        fputs(aliasLine, a);
        // Discard current line, otherwise the next strcat will append to it
        aliasLine[0] = '\0';
    }
    fclose(a);
}


/*
 * load aliases from file
 */
void loadAliases() {
    FILE *pFile;

    pFile = fopen(".aliases", "r");

    if (pFile == NULL) {
        printf("Alias file not found.\n");
        return;
    }

    int aliasIndex = 0;
    char buffer[1000];

    while (fgets(buffer, 1000, pFile) != NULL) {
        size_t length = strlen(buffer);

        if (length > 0 && buffer[length - 1] == '\n') {
            buffer[--length] = '\0';
        }

        char *string = malloc(sizeof(buffer));
        strcpy(string, buffer);
        printf("%s \n", string);

        char *pChr = strtok(string, " \t|><&;");

        if (pChr == NULL) { // when it's an empty line
            continue;
        }

        // Store alias tokens into line
        char* line[50];

        int tokenIndex = 0;
        while (pChr != NULL) {
            if (tokenIndex >= 50) {
                printf("Argument limit exceeded");
                break;
            }
            line[tokenIndex] = strdup(pChr);
            pChr = strtok(NULL, " \t|><&;");
            tokenIndex++;
        }

        // Use first token as name, the others as command tokens
        
        Alias* alias = malloc(sizeof(Alias));
        // set the name to the 0th token
        alias->name = strdup(line[0]);

        alias->numCommandTokens = 0;
        // skip over 0th token, which is the name
        for(int i=0; i<tokenIndex - 1; i++) {
            alias->commandTokens[i] = line[i+1];
            alias->numCommandTokens++;
        }

        alias->numLinkedCommands = 0;
        alias->visited = 0;
        aliasList[aliasIndex] = alias;

        aliasIndex++;
        if(tokenIndex < 2) { // alias file contained only one command on this line
            printf("Invalid alias detected in file\n");
            // undo borked alias
            aliasIndex--;
            aliasCommands[aliasIndex][tokenIndex] = NULL;
        }
    }
    linkAliases();
    fclose(pFile);
}
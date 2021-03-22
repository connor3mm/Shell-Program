#include "alias.h"


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
            aliasList[aliasIndex] = NULL;
        }
    }
    linkAliases();
    fclose(pFile);
}
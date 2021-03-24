#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct Alias {
    char *name;
    // command and parameters
    char *commandTokens[50];
    int numCommandTokens;
    // other aliases referenced by this alias
    struct Alias *linkedCommands[10];
    int numLinkedCommands;
    // only used for detecting loops
    int visited;
} Alias;
Alias *aliasList[10];

void addAlias(char **tokens);

void unAlias(char *name);

void printAlias();

void replaceAliases(char **tokens);

void saveAliases();

void loadAliases();

void rebuildAliasLinks();

void linkAliases();

int checkAliasLoop(Alias *current, int stack[]);

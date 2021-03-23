#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define HISTORY_LIMIT 20
char *history[HISTORY_LIMIT];
int currentHistorySize = 0;
int currentHistoryIndex = 0;
int oldestHistoryIndex = 0;


void saveHistory();

void loadHistory();

void getHistory(char *pString[51]);
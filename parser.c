#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

/*
 * This function will allow the user to input intoa command to the terminal
 */
void input_command(char* input) {


    // using delimiters to slip the input

    char *pChr = strtok (input, " \t|><&;");
    char *tokens[50];
    int index = 0;

    while (pChr != NULL) {

        tokens[index] = pChr;
        printf ("%s ", tokens[index]);
        index++;
        pChr = strtok (NULL, " \t|><&;");
    }
    putchar ('\n');
}
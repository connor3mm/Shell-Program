#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

/*
 * This function will print the current username and working directory before user input,
 * in the format: user@current_folder> without including the whole path
 */
void print_display_prompt() {
    // Get working directory. PATH_MAX declared directly to make it work on Cygwin.
    uint PATH_MAX = 4096;
    char current_working_dir[PATH_MAX];
    getcwd(current_working_dir, PATH_MAX);
    // Get last folder - we want to keep the prompt length down as per the specification
    // assume folder will never be null since every absolute Unix-style path needs a /
    char* folder = strrchr(current_working_dir, '/');
    // Get username
    struct passwd *passwd_entry = getpwuid(getuid());
    if (passwd_entry == NULL) {
        printf("Could not get user name.");
        exit(1);
    }
    // print prompt - add 1 to folder so it doesn't include the /
    printf("%s@%s> ", passwd_entry->pw_name, folder+1);
}

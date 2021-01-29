#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

int main(void) {
    // while true get next line (string)
    // get user name to display before command
    struct passwd *passwd_entry = getpwuid(getuid());
    if (passwd_entry == NULL) {
        printf("Could not get user name.");
        exit(1);
    }
    printf("Name: %s $ ", passwd_entry->pw_name);
    // pass line to parser.c to return a list of commands (strings)
    // send list of commands to command.c to execute
    return 0;
}

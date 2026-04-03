#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Only error message allowed to print
void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// char *tokenizeInput(char *line) {
//     char *token;
//     char *saveptr = line;
//     char export[5];
//     while ((token = strsep(&saveptr, " \t")) != NULL) {
//         /* code */
//     }
    

// }

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // Interactive Loop
    if (argc == 1) {
        while (1) {
            printf("wish> ");
            fflush(stdout); // Prompt print immediately

            // getline() reads the whole line
            read = getline(&line, &len, stdin);
            printf("size: %s", len);
            // If user hits Ctrl+D (EOF), exit gracefully
            if (read == -1) {
                break;
            }

            // Remove the newline character at the end of the input
            if (line[read - 1] == '\n') {
                line[read - 1] = '\0';
            }

            // Ignore empty inputs (user just hit enter)
            if (strlen(line) == 0) {
                continue;
            }

            // --- BUILT-IN COMMAND: exit ---
            if (strcmp(line, "exit") == 0) {
                exit(0);
            }

            // TODO: Phase 2 - Parse the input into arguments using strsep()
            // TODO: Phase 2 - fork() a child process
            // TODO: Phase 2 - Use execv() to run the command
            // TODO: Phase 2 - wait() for the child to finish

            // Echo what the user typed to prove it works:
            printf("You typed: %s\n", line);
        }
    } 
    // pass a file like ./wish batch.txt
    else if (argc == 2) {
        // TODO: Phase 4 - Open the file, read line by line instead of stdin
        printf("Batch mode not yet implemented.\n");
    } 
    // Too many arguments
    else {
        print_error();
        exit(1);
    }

    // Free the memory getline() allocated
    free(line);
    return 0;
}
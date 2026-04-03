#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Only error message allowed to print
void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char **tokenizeInput(char *line, int *argc_out) {
    int capacity = 10;
    int count = 0;
    char **tokens = malloc(sizeof(char *) * capacity);
    char *token;
    char *saveptr = line;

    if (tokens == NULL) {
        print_error();
        exit(1);
    }

    while ((token = strsep(&saveptr, " \t")) != NULL) {
        // Skip empty tokens caused by repeated spaces/tabs
        if (*token == '\0') {
            continue;
        }

        if (count >= capacity - 1) {
            capacity *= 2;
            char **temp = realloc(tokens, sizeof(char *) * capacity);
            if (temp == NULL) {
                free(tokens);
                print_error();
                exit(1);
            }
            tokens = temp;
        }
        tokens[count++] = token;
    }
    tokens[count] = NULL;

    if (argc_out != NULL) {
        *argc_out = count;
    }

    return tokens;
}

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

            // Tokenizes line, declare variable 
            int arg_count;
            char **args = tokenizeInput(line, &arg_count);
            
            
            if (arg_count == 0) { free(args); continue; } // Ignore empty input
            if (strcmp(args[0], "exit") == 0) { free(args); 
                exit; } // Built-in exit command.
            if (read == -1) { break; } // If user hits Ctrl+D (EOF), exit gracefully

            // Echo user input. proof it works:
            printf("Token: %s\n", args[0]);


            // TODO: Phase 2 - Parse the input into arguments using strsep()
            // TODO: Phase 2 - fork() a child process
            // TODO: Phase 2 - Use execv() to run the command
            // TODO: Phase 2 - wait() for the child to finish


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
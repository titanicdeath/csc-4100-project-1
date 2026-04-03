#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

// Only error message allowed to print
void print_error(void);
char *normalizeInput(char *line);
char **tokenizeInput(char *line, int *argc_out);
int builtin_exit(char **args, int argc);
int builtin_cd(char **args, int argc);
int builtin_path(char **args, int argc);
int run_builtin_if_match(char **args, int argc);
int execute_external(char **args, char *redirect_file);
void process_line(char *line);

// Built-in function pointer type
typedef int (*builtin_func)(char **args, int argc);

// Dispatch table entry
typedef struct {
    char *name;
    builtin_func func;
} builtin_entry;

// Global shell path list
char *path_list[100];
int path_count = 1;

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // Default search path
    path_list[0] = "/bin";

    // Interactive Loop
    if (argc == 1) {
        while (1) {
            printf("wish> ");
            fflush(stdout); // Prompt print immediately

            // getline() reads the whole line
            read = getline(&line, &len, stdin);

            if (read == -1) { break; } // If user hits Ctrl+D (EOF), exit gracefully
            if (read > 0 && line[read - 1] == '\n') { line[read - 1] = '\0'; } // Strip trailing newline from getline()
            process_line(line);
        }
    }
    // pass a file like ./wish batch.txt
    else if (argc == 2) {
        FILE *batch_file = fopen(argv[1], "r");

        if (batch_file == NULL) { print_error(); exit(1); } // If batch file cannot be opened, print error and exit

        while ((read = getline(&line, &len, batch_file)) != -1) {
            if (read > 0 && line[read - 1] == '\n') { line[read - 1] = '\0'; } // Strip trailing newline from getline()
            process_line(line);
        }
        fclose(batch_file);
    }
    
    else { print_error(); exit(1); } // Too many arguments

    // Free the memory getline() allocated
    free(line);
    return 0;
}

void print_error(void) {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void process_line(char *line) {
    pid_t child_pids[100];
    int child_count = 0;
    char *normalized = normalizeInput(line);
    char *segment;
    char *saveptr = normalized;

    if (normalized == NULL) { print_error(); exit(1); }

    while ((segment = strsep(&saveptr, "&")) != NULL) {
        int arg_count = 0;
        int redirect_count = 0;
        int redirect_index = -1;
        char *redirect_file = NULL;
        char **args = tokenizeInput(segment, &arg_count);

        // Ignore fully empty lines, but not empty command slots between & operators
        if (arg_count == 0) {
            if (saveptr != NULL || child_count > 0) { 
                print_error(); 
                free(args); 
                free(normalized); 
                return; 
            }
            free(args);
            continue;
        }

        for (int i = 0; i < arg_count; i++) {
            if (strcmp(args[i], ">") == 0) {
                redirect_count++;
                redirect_index = i;
            }
        }

        if (redirect_count > 1) { 
                print_error(); 
                free(args); 
                free(normalized); 
                return; 
            }

        if (redirect_count == 1) {
            if (redirect_index == 0 || redirect_index != arg_count - 2) { 
                print_error(); 
                free(args); 
                free(normalized); 
                return; 
            }
            redirect_file = args[arg_count - 1];
            args[redirect_index] = NULL;
            arg_count = redirect_index;
            if (arg_count == 0) { 
                print_error(); 
                free(args); 
                free(normalized); 
                return; 
            }
        }

        // Check built-ins first
        // Otherwise treat as external command
        if (run_builtin_if_match(args, arg_count)) {
            if (redirect_file != NULL) { 
                print_error(); 
                free(args); 
                free(normalized); 
                return; }
        }
        else {
            pid_t pid = execute_external(args, redirect_file);
            if (pid > 0) { child_pids[child_count++] = pid; }
        }

        // Free token array
        free(args);
    }

    for (int i = 0; i < child_count; i++) { waitpid(child_pids[i], NULL, 0); }
    free(normalized);
}

char *normalizeInput(char *line) {
    size_t len = strlen(line);
    size_t capacity = len * 3 + 1;
    char *normalized = malloc(capacity);
    size_t j = 0;

    if (normalized == NULL) { return NULL; }

    for (size_t i = 0; i < len; i++) {
        if (line[i] == '>' || line[i] == '&') {
            normalized[j++] = ' ';
            normalized[j++] = line[i];
            normalized[j++] = ' ';
        }
        else { normalized[j++] = line[i]; }
    }

    normalized[j] = '\0';
    return normalized;
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

    // Split by spaces/tabs using strsep()
    while ((token = strsep(&saveptr, " \t")) != NULL) {
        // Skip empty tokens caused by repeated spaces/tabs
        if (*token == '\0') {
            continue;
        }

        // Grow token array if needed
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

    // NULL-terminate for execv()
    tokens[count] = NULL;

    if (argc_out != NULL) {
        *argc_out = count;
    }

    return tokens;
}

// Built-in exit command
int builtin_exit(char **args, int argc) {
    if (argc != 1) { print_error(); return 1; }
    exit(0);
}

// Built-in cd command
int builtin_cd(char **args, int argc) {
    if (argc != 2) { print_error(); return 1; }
    if (chdir(args[1]) != 0) { print_error(); }
    return 1;
}

// Built-in path command
int builtin_path(char **args, int argc) {
    // Clear old path list
    path_count = 0;
    for (int i = 1; i < argc; i++) { path_list[path_count++] = args[i]; } // path with no extra arguments means no search paths
    return 1;
}

// Check dispatch table for built-in command match
int run_builtin_if_match(char **args, int argc) {
    builtin_entry builtins[] = {
        {"exit", builtin_exit},
        {"cd", builtin_cd},
        {"path", builtin_path}
    };

    int builtin_count = sizeof(builtins) / sizeof(builtins[0]);

    for (int i = 0; i < builtin_count; i++) {
        if (strcmp(args[0], builtins[i].name) == 0) {
            builtins[i].func(args, argc);
            return 1; // handled as built-in
        }
    }

    return 0; // not a built-in
}

// Run non-built-in external command
int execute_external(char **args, char *redirect_file) {
    char fullpath[1024];
    int found = 0;

    // Search each directory in path_list
    for (int i = 0; i < path_count; i++) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path_list[i], args[0]);

        if (access(fullpath, X_OK) == 0) { found = 1; break; }
    }

    // Command not found in any allowed path
    if (!found) { print_error(); return -1; }

    // Create child process
    pid_t pid = fork();

    if (pid < 0) {print_error(); return -1; }

    if (pid == 0) {
        if (redirect_file != NULL) {
            int fd = open(redirect_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0) { print_error(); _exit(1); }
            if (dup2(fd, STDOUT_FILENO) < 0) { print_error(); close(fd); _exit(1); }
            if (dup2(fd, STDERR_FILENO) < 0) { print_error(); close(fd); _exit(1); }
            close(fd);
        }

        // Child exec command
        execv(fullpath, args);

        // Only reached if execv fails
        print_error();
        _exit(1);
    }

    return pid;
}

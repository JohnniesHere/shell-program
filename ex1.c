//315428326

#include <stdio.h>      // Standard input/output library
#include <stdlib.h>     // Standard library for memory allocation and other utilities
#include <string.h>     // Library for string manipulation functions
#include <unistd.h>     // Library for POSIX operating system API functions
#include <sys/wait.h>   // Library for process control functions


#include "Stack.h"
#include "Alias.h"
#define MAX_CMD_LEN 1025    // Maximum command line length
#define MAX_ARGS 5          // Maximum number of arguments (command + 4 arguments + 1 for NULL)


// Global counters for commands, aliases, and script lines------------------------------------------------------------------
int cmd_count = 0;
int script_lines = 0;
int quoteCount = 0;

// Function prototypes--------------------------------------------------------------------------------------------------
void check_matching_quotes(const char *input, int* count);
void execute_user_command(char *command);
int handle_special_commands(const char *command);
void process_input();


//Process input function to handle user input and execute commands-------------------------------------------------------
void process_input() {
    char command[MAX_CMD_LEN];  // Buffer to store user input

    while (1) {
        // Print the prompt with counters
        printf("#cmd:%d|#alias:%d|#script lines:%d> ", cmd_count, alias_count, script_lines);
        if (fgets(command, sizeof(command), stdin) == NULL  || strlen(command) > MAX_CMD_LEN) {
            printf( "ERR: invalid input.\n");
            break;
        }

        // Check if user entered more than 1024 characters
        if (command[strlen(command) - 1] != '\n') {
            printf("ERR: Input exceeds 1024 characters limit.\n");
            // Clear the input buffer
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
            continue;
        }

        command[strcspn(command, "\n")] = '\0';  // Remove newline character from the command

        // Handle special commands
        int result = handle_special_commands(command);
        if (result == 1) {
            printf("%d", quoteCount);
            free_aliases();
            break;  // Exit the shell if the command is "exit_shell"
        } else if (result == 0) {
            continue;  // Special command was handled, continue to the next iteration
        }

        // Execute the user command if it's not a special command
        execute_user_command(command);

    }
}

// Function to split the command into arguments---------------------------------------------------------------------------
void split_command(char *command, char **args, int *arg_count) {
    *arg_count = 0;
    char *current = command;
    char *start = NULL;
    int in_quotes = 0;
    char quote_char = '\0';

    while (*current) {
        if (in_quotes) {
            if (*current == quote_char) {
                in_quotes = 0;
                *current = '\0';  // Replace closing quote with null terminator
                args[(*arg_count)++] = start;  // Add argument without quotes
                start = NULL;
            }
        } else {
            if (*current == '"' || *current == '\'') {
                in_quotes = 1;
                quote_char = *current;
                start = current + 1;  // Skip the opening quote
            } else if (*current != ' ') {
                if (start == NULL) {
                    start = current;
                }
            } else {
                if (start) {
                    *current = '\0';  // Replace space with null terminator
                    args[(*arg_count)++] = start;
                    start = NULL;
                }
            }
        }
        current++;
    }
    args[(*arg_count)++] = start;
    args[*arg_count] = NULL;  // Null-terminate the arguments array

}

// Function to execute a command using fork and execvp-------------------------------------------------------------------
void execute_command(char **args, char* copy) {
    if (args[0] == NULL ) {  // Ensure there's a command to execute
        printf("ERR: Empty Command\n");
        return;
    }
    pid_t pid = fork();  // Create a new process
    if (pid == 0) {  // Child process
        if (execvp(args[0], args) == -1) {  // Execute the command
            perror("exec");  // Print error if execvp fails
            _exit(EXIT_FAILURE);  // Exit child process if execvp fails
        }
    } else if (pid < 0) {  // Fork failed
        perror("fork");
        _exit(EXIT_FAILURE);
    } else {  // Parent process
        int status;
        waitpid(pid, &status, 0);  // Wait for the child process to finish
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            check_matching_quotes(copy, &quoteCount);
            // Command executed successfully, increment command count
            cmd_count++;
        }
    }
}

// Function to handle script files------------------------------------------------------------------------------------------
void handle_script(const char *filename) {
    FILE *file = fopen(filename, "r");  // Open the script file
    cmd_count++;
    if (file == NULL) {
        perror("ERR");  // Print error if file cannot be opened
        _exit(EXIT_FAILURE);
    }
    char line[MAX_CMD_LEN];  // Buffer to store each line from the script
    if (fgets(line, sizeof(line), file) == NULL && strcmp(line, "#!/bin/bash") != 0) {
        printf("ERR: Invalid script file\n");
       return;
    }
    script_lines++;// For the #!/bin/bash line
    while (fgets(line, sizeof(line), file)) {  // Read each line from the script
        script_lines++;
        line[strcspn(line, "\n")] = '\0';  // Remove newline character from the line
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }

        // Handle special commands
        int result = handle_special_commands(line);
        if (result == 1) {
            printf("%d", quoteCount);
            break;  // Exit the shell if the command is "exit_shell"
        } else if (result == 0) {
            continue;  // Special command was handled, continue to the next iteration
        }

        // Execute the user command if it's not a special command
        execute_user_command(line);
    }
    fclose(file);
}

// Function to check if the command contains quotes----------------------------------------------------------------------
void check_matching_quotes(const char *input, int* count) {
    /* At first, I thought I need to count how many legal pairs of quotation have been used
     * i.e. if the input is alias ll='echo "hello"' it would've count as 2
     * if the input is echo "i can't" I won't would count as 1
     * if the input is echo "1" "2" "3" would count as 3
     * due to shortage of time i added breaks after the first encounter of legal quotation mark instead of rewriting it. */
    char *new_string = (char*)malloc(strlen(input) + 1);
    int index = 0;
    // Check if memory allocation was successful
    if (new_string == NULL) {
        perror("malloc");
        _exit(EXIT_FAILURE);
    }
    // Copy the input string to the new string
    strcpy(new_string, input);
    StackNode* stack = NULL;  // Initialize the stack
    char temp;
    while (new_string[index]) {
        if (new_string[index] == '"' || new_string[index] == '\'') {
            if (is_empty(stack)) {
                push(&stack, new_string[index]);  // Push the quote onto the stack

            } else {
                char top_quote = stack->data;
                if((new_string[index]== '"' && top_quote == '"') || (new_string[index] == '\'' && top_quote == '\'')){
                    pop(&stack);
                    (*count)++;
                    temp = '\0';
                    break;
                }
                else if((temp == '"' && new_string[index]== '"') || (temp == '\'' && new_string[index] == '\'')){
                    (*count)++;
                    temp = '\0';
                    break;
                }else if ((new_string[index] == '"' && top_quote == '\'') || (new_string[index] == '\'' && top_quote == '"')) {
                    temp = new_string[index];  // Mismatched quotes
                }
            }
        }
        index++;
    }

    while (!is_empty(stack)) {
        pop(&stack);  // Clean up the stack
    }
    free(new_string);
}

// Function to handle special commands----------------------------------------------------------------------------------
int handle_special_commands(const char *command) {
    if (strcmp(command, "exit_shell") == 0) {
        return 1;  // Signal to exit the shell
    }

    if (strncmp(command, "alias ", 6) == 0) {
        check_matching_quotes(command, &quoteCount);
        char *name = strtok((char*)command + 6, "=");  // Get the alias name
        char *cmd = strtok(NULL, "'");  // Get the alias command
        if (name && cmd) {
            add_alias(name, cmd);  // Add the new alias
            cmd_count++;
        }
        return 0;
    }

    if (strcmp(command, "alias") == 0) {
        print_aliases();  // Print all defined aliases
        cmd_count++;
        return 0;
    }

    if (strncmp(command, "unalias ", 8) == 0) {
        char *name = strtok((char*)command + 8, "\0");  // Get the alias name
        if (name) {
            unalias(name);  // Remove the specified alias
            cmd_count++;
        }
        return 0;
    }

    if (strncmp(command, "source ", 7) == 0) {
        char *filename = strtok((char*)command + 7, "\0");
        if (filename) {
            handle_script(filename);  // Execute the script file
        } else {
            printf("ERR: File not fount\n");
            return -1;
        }
        return 0;
    }

    return -1;  // Command not handled here
}

// Function to execute user commands------------------------------------------------------------------------------------
void execute_user_command(char *command) {
    char *copy = strdup(command);
    char *args[MAX_ARGS + 1];  // Array to store command arguments
    int arg_count;
    split_command(command, args, &arg_count);  // Split the command into arguments

    if (arg_count == 0) {
        return;  // Ignore empty commands
    }

    if (arg_count >= MAX_ARGS + 1) {  // Command plus 4 arguments is allowed

        printf("Error: Too many arguments\n");
        return;  // Skip commands with more than 4 arguments
    }

    Alias *alias = (args[0] != NULL) ? find_alias(args[0] ) : NULL;  // Check if the command is an alias
    if (alias != NULL) {
        char alias_command[MAX_CMD_LEN];
        strcpy(alias_command, alias->command);  // Get the alias command
        for (int i = 1; i < arg_count; i++) {
            strncat(alias_command, " ", MAX_CMD_LEN - strlen(alias_command) - 1);
            strncat(alias_command, args[i], MAX_CMD_LEN - strlen(alias_command) - 1);
        }
        split_command(alias_command, args, &arg_count);  // Split the alias command into arguments
    }

    if (arg_count > 0) {
        check_matching_quotes(command, &quoteCount);
        execute_command(args, copy);  // Execute the command
    }
    free(copy);
}


// Main function---------------------------------------------------------------------------------------------------------
int main() {

    process_input();  // Process user input and execute commands
    return 0;
}

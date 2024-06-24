//315428326

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#include "Stack.h"
#include "Alias.h"
#include "runData.h"
#include "Jobs.h"

#define MAX_CMD_LEN 1025    // Maximum command line length
#define MAX_ARGS 5          // Maximum number of arguments (command + 4 arguments + 1 for NULL)


// Function prototypes--------------------------------------------------------------------------------------------------
void process_input(runData *data);

void split_command(char *command, char **args, int *arg_count);

int execute_command(char **args, char *copy, int background, const char *error_file, runData *data);

void handle_script(const char *filename, runData *data);

void check_matching_quotes(const char *input, runData *data);

int handle_special_commands(const char *command, runData *data);

void execute_user_command(char *command, runData *data, int saved_stderr, const char *error_file);

void handler_sigchld(int sig);

int redirect_stderr(const char *error_file);

void restore_stderr(int saved_stderr);

int cmd_count = 0;
int arcount = 0;

void handler_sigchld(int sig) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Command executed successfully, increment command count
            cmd_count++;
            //wait(NULL);
        }
        remove_completed_jobs();
    }
}

// Function to redirect stderr to a file
int redirect_stderr(const char *error_file) {
    if (error_file) {
        int fd = open(error_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open");
            return -1;
        }
        int saved_stderr = dup(STDERR_FILENO);
        if (dup2(fd, STDERR_FILENO) == -1) {
            perror("dup2");
            close(fd);
            return -1;
        }
        close(fd);
        return saved_stderr;
    }
    return -1;
}

// Function to restore stderr after redirection
void restore_stderr(int saved_stderr) {
    if (saved_stderr != -1) {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
    }
}


void process_input(runData *data) {
    char command[MAX_CMD_LEN];  // Buffer to store user input
    int saved_stderr = -1;  // Variable to store the original stderr

    while (1) {
        // Print the prompt with counters
        printf("#cmd:%d|#alias:%d|#script lines:%d> ", cmd_count, alias_count, data->script_lines);
        if (fgets(command, sizeof(command), stdin) == NULL) {
            continue;
        }

        // Check for 2> for error redirection
        char *redir = strstr(command, "2>");
        char *error_file = NULL;
        if (redir) {
            *redir = '\0';  // Split the command at 2>
            error_file = redir + 2;

            while (*error_file == ' ') error_file++;  // Skip spaces
            //remove '\n' from the end of the error file
            error_file[strcspn(error_file, "\n")] = '\0';
            char *end = redir - 1;
            while (end > command && *end == ' ') {
                *end = '\0';  // Remove trailing spaces before 2>
                end--;
            }
            //Wrote this code to not count "'"2>" and "error_file" as arguments, saw the answer in the forum too late, so I used this quick patch for now.
            arcount ++;
            saved_stderr = redirect_stderr(error_file);  // Redirect stderr
        }

        if (strlen(command) > MAX_CMD_LEN) {
            fprintf(stderr, "ERR: Input exceeds 1024 characters limit.\n");
            // Clear the input buffer
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
            restore_stderr(saved_stderr);  // Restore stderr
            continue;
        }

        command[strcspn(command, "\n")] = '\0';  // Remove newline character from the command

        // Handle special commands
        int result = handle_special_commands(command, data);
        if (result == 1) {
            kill_all_jobs();
            printf("%d", data->quoteCount);
            free_aliases();
            restore_stderr(saved_stderr);  // Restore stderr
            break;  // Exit the shell if the command is "exit_shell"
        } else if (result == 0) {
            restore_stderr(saved_stderr);  // Restore stderr
            continue;  // Special command was handled, continue to the next iteration
        }

        // Execute the user command if it's not a special command
        execute_user_command(command, data, saved_stderr, error_file);

        // Restore stderr after command execution
        arcount =  0;
        restore_stderr(saved_stderr);
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
    if (start) {
        args[(*arg_count)++] = start;
    }
    args[*arg_count] = NULL;  // Null-terminate the arguments array
}

// Function to execute a command using fork and execvp-------------------------------------------------------------------
int execute_command(char **args, char *copy, int background, const char *error_file, runData *data) {
    if (args[0] == NULL) {  // Ensure there's a command to execute
        fprintf(stderr,"ERR: Empty Command\n");
        return 0;
    }
    pid_t pid = fork();  // Create a new process

    if (pid == 0) {  // Child process
        if (error_file) {
            int fd = open(error_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                perror("open");
                _exit(EXIT_FAILURE);
            }
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        if (execvp(args[0], args) == -1) {  // Execute the command
            perror("exec");  // Print error if execvp fails
            _exit(EXIT_FAILURE);  // Exit child process if execvp fails
        }
    } else if (pid < 0) {  // Fork failed
        perror("fork");
        _exit(EXIT_FAILURE);
    } else {  // Parent process
        if (background) {
            add_job(pid, copy);
        } else {
            int status;
            waitpid(pid, &status, 0);  // Wait for the child process to finish
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                if(copy != NULL) {
                    check_matching_quotes(copy, data);
                }
                // Command executed successfully, increment command count
                if(handle_special_commands(copy,data) == -1) {
                    cmd_count++;
                }
                return 1;
            }
        }
    }
    return 0;
}

// Function to handle script files------------------------------------------------------------------------------------------
void handle_script(const char *filename, runData *data) {
    int saved_stderr = -1;  // Variable to store the original stderr
    FILE *file = fopen(filename, "r");  // Open the script file
    if (file == NULL) {
        perror("ERR");  // Print error if file cannot be opened
        return;
    }
    cmd_count++;
    char line[MAX_CMD_LEN];  // Buffer to store each line from the script
    if (fgets(line, sizeof(line), file) == NULL && strcmp(line, "#!/bin/bash") != 0) {
        fprintf(stderr,"ERR: Invalid script file\n");
        return;
    }
    data->script_lines++;// For the #!/bin/bash line
    //read each line of the file till the end of the file
    while (fgets(line, sizeof(line), file)) {  // Read each line from the script
        data->script_lines++;
        //if ch == EOF break
        if(feof(file)){
            break;
        }
        line[strcspn(line, "\n")] = '\0';  // Remove newline character from the line
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }

        // Check for 2> for error redirection
        char *redir = strstr(line, "2>");
        char *error_file = NULL;
        if (redir) {
            *redir = '\0';  // Split the command at 2>
            error_file = redir + 2;

            while (*error_file == ' ') error_file++;  // Skip spaces
            //remove '\n' from the end of the error file
            error_file[strcspn(error_file, "\n")] = '\0';
            char *end = redir - 1;
            while (end > line && *end == ' ') {
                *end = '\0';  // Remove trailing spaces before 2>
                end--;
            }

            saved_stderr = redirect_stderr(error_file);  // Redirect stderr
        }

        // Check if user entered more than 1024 characters
        if (strlen(line) > MAX_CMD_LEN) {
            fprintf(stderr, "ERR: invalid input.\n");
            restore_stderr(saved_stderr);  // Restore stderr
            break;
        }
        if (strlen(line) > MAX_CMD_LEN) {
            fprintf(stderr, "ERR: Input exceeds 1024 characters limit.\n");
            // Clear the input buffer
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
            restore_stderr(saved_stderr);  // Restore stderr
            continue;
        }

        line[strcspn(line, "\n")] = '\0';  // Remove newline character from the command

        // Handle special commands
        int result = handle_special_commands(line, data);
        if (result == 1) {
            printf("%d", data->quoteCount);
            free_aliases();
            restore_stderr(saved_stderr);  // Restore stderr
            break;  // Exit the shell if the command is "exit_shell"
        } else if (result == 0) {
            restore_stderr(saved_stderr);  // Restore stderr
            continue;  // Special command was handled, continue to the next iteration
        }

        // Execute the user command if it's not a special command
        execute_user_command(line, data, saved_stderr, error_file);

        // Restore stderr after command execution
        restore_stderr(saved_stderr);
    }

    fclose(file);
}

// Function to check if the command contains quotes----------------------------------------------------------------------
void check_matching_quotes(const char *input, runData *data) {
    /* At first, I thought I need to count how many legal pairs of quotation have been used
     * i.e. if the input is alias ll='echo "hello"' it would've count as 2
     * if the input is echo "i can't" I won't would count as 1
     * if the input is echo "1" "2" "3" would count as 3
     * due to shortage of time i added breaks after the first encounter of legal quotation mark instead of rewriting it. */
    //if the input is NULL then return
    if (input == NULL) {
        return;
    }
    char *new_string = (char *) malloc(strlen(input) + 1);
    int index = 0;
    // Check if memory allocation was successful
    if (new_string == NULL) {
        perror("malloc");
        _exit(EXIT_FAILURE);
    }
    // Copy the input string to the new string
    strcpy(new_string, input);
    StackNode *stack = NULL;  // Initialize the stack
    char temp ='\0';  // Temporary variable to store mismatched quotes
    while (new_string[index]) {
        if (new_string[index] == '"' || new_string[index] == '\'') {
            if (is_empty(stack)) {
                push(&stack, new_string[index]);  // Push the quote onto the stack

            } else {
                char top_quote = stack->data;
                if ((new_string[index] == '"' && top_quote == '"') ||
                    (new_string[index] == '\'' && top_quote == '\'')) {
                    pop(&stack);
                    data->quoteCount++;
                    temp = '\0';
                    break;
                } else if ((temp == '"' && new_string[index] == '"') || (temp == '\'' && new_string[index] == '\'')) {
                    data->quoteCount++;
                    temp = '\0';
                    break;
                } else if ((new_string[index] == '"' && top_quote == '\'') ||
                           (new_string[index] == '\'' && top_quote == '"')) {
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
int handle_special_commands(const char *command, runData *data) {
    if (strcmp(command, "exit_shell") == 0) {

        return 1;  // Signal to exit the shell
    }

    if (strcmp(command, "jobs") == 0) {
        print_jobs();
        cmd_count++;
        return 0;
    }

    if (strncmp(command, "alias ", 6) == 0) {
        check_matching_quotes(command, data);
        char *name = strtok((char *) command + 6, "=");  // Get the alias name
        char *cmd = strtok(NULL, "'");
        // Get the alias command
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
        char *name = strtok((char *) command + 8, "\0");  // Get the alias name
        if (name) {
            unalias(name);  // Remove the specified alias
            cmd_count++;
        }
        return 0;
    }

    if (strncmp(command, "source ", 7) == 0) {
        char *filename = strtok((char *) command + 7, "\0");
        if (filename) {
            handle_script(filename, data);  // Execute the script file
        } else {
            fprintf(stderr,"ERR: File not found\n");
            return -1;
        }
        return 0;
    }

    return -1;  // Command not handled here
}


int is_or_operator_in_quotes(const char *command) {
    int in_quotes = 0;
    char quote_char = '\0';
    int found_or_operator_in_quotes = 0;

    while (*command) {
        if (in_quotes) {
            if (*command == quote_char) {
                in_quotes = 0;
            } else if (*command == '|' && *(command + 1) == '|') {
                found_or_operator_in_quotes = 1;
            }
        } else {
            if (*command == '"' || *command == '\'') {
                in_quotes = 1;
                quote_char = *command;
            }
        }
        command++;
    }

    return found_or_operator_in_quotes;
}

int is_and_operator_in_quotes(const char *command) {
    int in_quotes = 0;
    char quote_char = '\0';
    int found_and_operator_in_quotes = 0;

    while (*command) {
        if (in_quotes) {
            if (*command == quote_char) {
                in_quotes = 0;
            } else if (*command == '&' && *(command + 1) == '&') {
                found_and_operator_in_quotes = 1;
            }
        } else {
            if (*command == '"' || *command == '\'') {
                in_quotes = 1;
                quote_char = *command;
            }
        }
        command++;
    }

    return found_and_operator_in_quotes;
}

void execute_user_command(char *command, runData *data, int saved_stderr, const char *error_file) {
    char *copy = strdup(command);
    char *args[MAX_ARGS + 1];  // Array to store command arguments
    int arg_count = 0;
    int background = 0;
    char *sep;
    int status = 0;
    char *check_quotes = NULL;
    char *check_quotes_or = NULL;

    // Check for & at the end of the command for background execution
    char *check_and = strstr(command, "&&");
    char *check_or = strstr(command, "||");
    if(check_and == NULL && check_or == NULL) {
        if (command[strlen(copy) - 1] == '&') {
            background = 1;
            copy[strlen(copy) - 1] = '\0';  // Remove & from the command
            status = 1;
        }
    }


    // Check for brackets
    char *start_bracket = strchr(copy, '(');
    char *end_bracket = strchr(copy, ')');
    if (start_bracket && end_bracket) {
        // Remove the brackets from the command
        memmove(start_bracket, start_bracket + 1, strlen(start_bracket));
        memmove(end_bracket - 1, end_bracket, strlen(end_bracket) + 1);
    }

    char *or_command = NULL;
    if (is_or_operator_in_quotes(copy) == 0 ) {
        // Check for || operator
        sep = strstr(copy, "||");

        if (sep) {
            *sep = '\0';
            sep += 2;
            check_quotes_or = strdup(copy);
            status = 2;
            while (*sep == ' ') sep++;  // Skip spaces
            or_command = sep;
        }
    }

    if (is_and_operator_in_quotes(copy) == 0) {
    // Check for && operator
    sep = strstr(copy, "&&");
    if (sep) {
        *sep = '\0';
        sep += 2;
        check_quotes = strdup(copy);
        while (*sep == ' ') sep++;  // Skip spaces

        split_command(copy, args, &arg_count);
        // If the first command is a special command, handle it
        if (handle_special_commands(copy, data) != -1) {
            // If the second command is also a special command, handle it
            if (handle_special_commands(sep, data) == -1) {
                // If the second command is not a special command, execute it
                execute_user_command(sep, data, saved_stderr, error_file);
            }
        } else {
            if(arg_count > MAX_ARGS) {
                fprintf(stderr,"ERR: Too many arguments\n");
                status = 5;
            }
            // If the first command is not a special command, execute it
            if ( status != 5 && execute_command(args, copy, background, error_file, data)) {
                check_matching_quotes(check_quotes, data);
                // If the second command is a special command, handle it
                if (handle_special_commands(sep, data) == -1) {
                    // If the second command is not a special command, execute it
                    execute_user_command(sep, data, saved_stderr, error_file);
                }
            }
                // If the first command wasn't special and resulted in an error, don't execute the second command and skip to the third
            else if (or_command) {
                if (handle_special_commands(or_command, data) == -1) {
                    execute_user_command(or_command, data, saved_stderr, error_file);
                }
            }else{
                if (check_quotes!= NULL) {
                    free(check_quotes);
                }
                if (check_quotes_or != NULL) {
                    free(check_quotes_or);
                }
                if (copy != NULL) {
                    free(copy);
                }
                return;
            }
        }
        status = 3;
     }
    }


    //if(sep != NULL){
        //split_command(sep, args, &arg_count);
   // }else {
        // Split the command into arguments
        split_command(copy, args, &arg_count);
  //  }

    if (arg_count == 0) {
        if (check_quotes!= NULL) {
            free(check_quotes);
        }
        if (check_quotes_or != NULL) {
            free(check_quotes_or);
        }
        if (copy != NULL) {
            free(copy);
        }
        return;  // Ignore empty commands
    }

    //if arcount > 0 , arg_count +2


    if (arg_count > MAX_ARGS) {  // Command plus 4 arguments is allowed
        fprintf(stderr,"ERR: Too many arguments\n");
        if(status ==2){
            execute_user_command(or_command, data, saved_stderr, error_file);
        }
        if (check_quotes!= NULL) {
            free(check_quotes);
        }
        if (check_quotes_or != NULL) {
            free(check_quotes_or);
        }
        if (copy != NULL) {
            free(copy);
        }
        return;  // Skip commands with more than 4 arguments
    }

    // Handle alias
    Alias *alias = (args[0] != NULL) ? find_alias(args[0]) : NULL;  // Check if the command is an alias
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
        if (status != 3) {
            if (status == 1 || status == 0) {
                execute_command(args, command, background, error_file, data);
            } else {
                if(check_quotes_or != NULL){
                    if (!execute_command(args, check_quotes_or, background, error_file, data) &&
                        handle_special_commands(or_command, data) == -1) {
                        execute_user_command(or_command, data, saved_stderr, error_file);
                         } else {
                        check_matching_quotes(check_quotes, data);
                    }
                }else if (!execute_command(args, check_quotes_or, background, error_file, data) &&
                          handle_special_commands(or_command, data) == -1) {
                    execute_user_command(or_command, data, saved_stderr, error_file);
                } else {
                    check_matching_quotes(check_quotes, data);
                }
            }
        }
    }
    if (check_quotes!= NULL) {
        free(check_quotes);
    }
    if (check_quotes_or != NULL) {
        free(check_quotes_or);
    }
    if (copy != NULL) {
        free(copy);
    }
}

// Main function---------------------------------------------------------------------------------------------------------
int main() {
    struct runData *data = (struct runData *) malloc(sizeof(struct runData));
    struct Job *jobs = (struct Job *) malloc(sizeof(struct Job));
    jobs->id = 0;
    data->quoteCount = 0;
    data->script_lines = 0;

    struct sigaction act;
    act.sa_handler = handler_sigchld;
    act.sa_flags = SA_RESTART;

    // Initialize intmask and set it to act.sa_mask
    sigset_t intmask;
    sigemptyset(&intmask);
    act.sa_mask = intmask;

    sigaction(SIGCHLD, &act, NULL);
    process_input(data);  // Process user input and execute commands

    if(data != NULL) {
        free(data);
    }
    if(jobs != NULL) {
        free(jobs);
    }
    return 0;
}
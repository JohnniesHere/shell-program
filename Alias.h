

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef UNTITLED6_ALIAS_H
#define UNTITLED6_ALIAS_H

#define MAX_ALIAS_LEN 1017//1025 - alias - (= )- (' )- (' )= 1017
int alias_count = 0;

// Structure for alias linked list
typedef struct Alias {
    char name[MAX_ALIAS_LEN];     // Alias name
    char command[MAX_ALIAS_LEN];  // Command associated with the alias
    struct Alias *next;           // Pointer to the next alias in the list
} Alias;

Alias *aliases = NULL;  // Head of the alias linked list

// Function to add a new alias or update an existing alias
void add_alias(const char *name, const char *command) {
    Alias *current = aliases;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            // Alias with the same name exists, update its command
            strcpy(current->command, command);
            return;
        }
        current = current->next;
    }

    // Alias does not exist, create a new alias
    Alias *new_alias = (Alias *)malloc(sizeof(Alias));  // Allocate memory for the new alias
    if (!new_alias) {
        perror("malloc");
        _exit(EXIT_FAILURE);
    }
    strcpy(new_alias->name, name);         // Copy the alias name
    strcpy(new_alias->command, command);   // Copy the alias command
    new_alias->next = aliases;             // Insert the new alias at the head of the list
    aliases = new_alias;
    alias_count++;                         // Increment alias count
}

// Function to find an alias by name
Alias* find_alias(const char *name) {
    Alias *current = aliases;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;  // Return the alias if found
        }
        current = current->next;
    }
    return NULL;  // Return NULL if the alias is not found
}

// Function to print all defined aliases
void print_aliases() {
    Alias *current = aliases;
    while (current != NULL) {
        printf("%s='%s'\n", current->name, current->command);
        current = current->next;
    }
}

// Function to remove an alias by name
void unalias(const char *name) {
    Alias *prev = NULL;
    Alias *current = aliases;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            if (prev == NULL) {
                aliases = current->next;  // Update head of the list if removing the first node
            } else {
                prev->next = current->next;  // Update pointers if removing a node in the middle or end
            }
            free(current);  // Free memory for the removed alias
            alias_count--;// Decrement alias count
            return;
        }
        prev = current;
        current = current->next;
    }
    printf("ERR: Alias '%s' not found\n", name);  // Print message if alias not found
}

// Function to free all allocated alias structures
void free_aliases() {
    Alias *current = aliases;
    while (current != NULL) {
        Alias *temp = current;
        current = current->next;
        free(temp);
    }
}

#endif //UNTITLED6_ALIAS_H

//315428326
#include <stdlib.h>
#ifndef UNTITLED6_STACK_H
#define UNTITLED6_STACK_H
typedef struct StackNode {
    char data;
    struct StackNode* next;
} StackNode;

// Function to create a new node
StackNode* create_node(char data) {
    StackNode* node = (StackNode*)malloc(sizeof(StackNode));
    if (node == NULL) {
        perror("malloc");
        return NULL; // Indicate an error by returning NULL
    }
    node->data = data;
    node->next = NULL;
    return node;
}

// Function to push a new element onto the stack
void push(StackNode** top, char data) {
    StackNode* node = create_node(data);
    node->next = *top;
    *top = node;
}

// Function to pop an element from the stack
char pop(StackNode** top) {
    if (*top == NULL) {
        return '\0';  // Return null character if stack is empty
    }
    StackNode* temp = *top;
    *top = (*top)->next;
    char popped = temp->data;
    free(temp);
    return popped;
}

// Function to check if the stack is empty
int is_empty(StackNode* top) {
    return top == NULL;
}

#endif //UNTITLED6_STACK_H

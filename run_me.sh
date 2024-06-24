#!/bin/bash

# Compile the ex1.c program with the -Wall flag
# In run_me.sh
gcc -Wall ex2.c -o ex2

# Check if compilation was successful
if [ $? -eq 0 ]; then
    #valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./ex1
	./ex2
else
    echo "Compilation failed. Please check your code."
fi
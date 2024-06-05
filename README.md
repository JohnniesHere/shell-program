Authorize: Jonathan Elgarisi

\## Shell Program This is a simple shell program written in C for
Linux-based machines. It reads commands from the user or a script file,
executes them, and handles special commands like \"alias\", \"unalias\",
and \"source\". It also counts the number of commands, script lines, and
commands that contains quotes. It uses a stack to check for quotes in
the input.

\## Features - Execute commands from user input or a script file. -
Handle special commands: \"alias\", \"unalias\", and \"source\". - Count
the number of commands, script lines, and commands that contain
quotes. - Check for matching quotes in the input using a stack.

\## How to Run To compile the program, use the following command:

#!/bin/bash gcc -Wall ex1.c -o ex1

Or use the run_me.sh script file as an alternative to compiling it
yourself.

\## How to Use After successfully launching the script or the command
lines provided above you can enter Linux commands or using one of the
special commands.

\*Special Commands: -alias: creates a shortcut for a command in the
format alias \<name\>=\'\<command\>\' and will save it in a linked-list
structure. In order to see the aliases list enter the word alias. (i.e.
alias ll=\'ls -l\' will assign ll as a shortcut for the command ls -l.
then, typing alias will result with the output: ll=\'ls -l\') -unalias:
to remove a shortcut from the aliases list (i.e. unalias ll will remove
the shortcut ll from the aliases linked-list) -source: running script
files, in order to use this command enter this format source
\<script_file_name.including_extention\> (i.e. source script.sh will run
the script.sh file located at the same folder as this shell)
-exit_shell: closes the shell program and prints the number of commands
that contained quotes.

\## Output According to your commands. Errors will be provide
information if occurs.

\## Files - ex1.c: The main file containing the shell program. -
Stack.h: Contains the definition and functions for a stack data
structure used for checking matching quotes. - Alias.h: Contains the
definition and functions for handling command aliases. - run_me.sh: A
script file to run the shell program. - README.txt: The text file
you\'re currently reading.

\## License MIT- https://choosealicense.com/licenses/mit/
"# shell-program" 

Authorize: Jonathan Elgarisi



## Shell Program
This is a simple shell program written in C for Linux-based machines. It reads commands from the user or a script file, executes them, and handles special commands like "alias", "unalias", and "source". It also counts the number of commands, script lines, and commands that contains quotes. It uses a stack to check for quotes in the input.


## Features
- Execute commands from user input or a script file.
- Handle special commands: "alias", "unalias", and "source".
- Count the number of commands, script lines, and commands that contain quotes.
- Check for matching quotes in the input using a stack.
- Support for && and || operators.
- Support for & operator.
- Support for 2> operator.
- Support for jobs command.


## How to Run
To compile the program, use the following command: 

#!/bin/bash
gcc -Wall ex1.c -o ex1

Or use the run_me.sh script file as an alternative to compiling it yourself.


## How to Use
After succesfully launching the script or the command lines provided above you can enter Linux commands or using one of the special commands.

*Special Commands:
-alias: creates a shortcut for a command in the format alias <name>='<command>' and will save it in a linked-list strcture.
        In order to see the aliases list enter the word alias.
        (i.e. alias ll='ls -l' will assaign ll as a shortcut for the command ls -l.
            then, typing alias will result with the output: ll='ls -l')
-unalias: to remove a shortcut from the aliases list
        (i.e. unalias ll will remove the shortcut ll from the aliases linked-list)
-source: running script files, in order to use this command enter this format source <script_file_name.including_extention>
        (i.e. source script.sh will run the script.sh file located at the same folder as this shell)
-exit_shell: closes the shell program and prints the number of commands that contained quotes.
-jobs: displays all processes running in the background.
        Example:
        sleep 50 & (repeated 3 times): Starts 3 background processes.
        jobs: Shows all running background processes with their PIDs.
            [1]               sleep 50 &
            [2]               sleep 50 &
            [3]               sleep 50 &


##Additional Features:
- && and || operators:
        These operators allow conditional execution of commands.
        &&: Command2 will run only if Command1 successfully executed.
        ||: Command2 will run only if Command1 failed to execute.
                Example:
                asdf && ls: asdf is not a valid command, so ls will not be executed.
                asdf || ls: asdf is not a valid command, so ls will be executed.

- & operator:
        The & character at the end of a command will run the command in the background, allowing the user to run more   commands immediately.
                Example:
                sleep 5: Runs sleep for 5 seconds, and the shell is "stuck" during this time.
                sleep 5 &: Runs sleep for 5 seconds in the background, allowing immediate command input.

- 2> operator:
        Redirects error output (stderr) to a file.
                Example:
                command 2> error_log: Runs the command, writing any error messages to the file error_log.
                aaaa 2> E: aaaa is not a valid command, so its error message is written to E. cat E prints the error message from the file.
                (aaa && ls) 2> EE: If aaa is invalid, its error message is written to EE, and ls will not be executed.


## Output
According to your commands.
Errors will be provide information if occurs.


## Files
- ex1.c: The main file containing the shell program.
- Stack.h: Contains the definition and functions for a stack data structure used for checking matching quotes.
- Alias.h: Contains the definition and functions for handling command aliases.
- Jobs.h: Contains the definition and functions for handling background jobs.
- runData.h: Contains the definition for tracking script lines and quote counts.
- run_me.sh: A script file to run the shell program.
- README.txt: The text file you're currently reading.



## License
MIT- https://choosealicense.com/licenses/mit/

# Shell
a linux based mini shell, can perform all operation. All function funtions are new created , Signal handling .
In this shell we can 
- execute commands with multiple arguments. 
    shell> Command arg1 arg2 arg3
- Maintain multiple processes running in background mode simultaneously
- Redirect the input of a command from a file. 
		shell> Command < input_file
- Redirect the output of a command to a file.
		shell> Command > output_file
- Implement command filters,, redirect the stdout of one command to stdin of another using pipes
- Signal handling is done.
- Any signal should be delivered to the entire process-group of the immediate child process, not just to the immediate child   process. 
- Many other functionalities implemented which are in linux terminal(linux shell).

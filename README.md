MyShell Simple Shell
==============

Simple recreation of the Unix shell written in C.

Created by Connor Dye as an California Polytechnic University Project.

Features
--------

* Basic commands: `exit`, `pwd`, `clear` and `cd` are supported from the parent's process
* Program invocation with forking and child processes
* supports I/O redirection (uses `dup2` system call to modify file descriptors)
* Pipelining implemented (`<cmd1> | <cmd2>`) via `pipe` and `dup2` syscalls. Multiple pipelining is supported.
* SIGINT signal handling is supported when Ctrl-C is pressed (shell is not exited)
* libmush.a handles the parsing in the shell
* supports interactive and batch processing. If shell is run with no argument it will read commands from stdin, otherwise it will take its commands from an argument

## Notes 
- regarding redirection, output files for redirection will be truncated to zero length (overwritten) if they currently exist. If they need to be created, read and write permissions are offered
- regarding the cd command, if the argument is absent the shell will change to the home directory; if the argument is present it will change to the argument directory
- this shell handles unexecutable commands by printing an error message and returning to the prompt (e.g non-existent input files or commands, failure of system calls, etc.
- after a SIGINT, the shell waits for children to terminate before returning to the shells prompt

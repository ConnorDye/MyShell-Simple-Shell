Simple Shell
==============

Simple shell written in C.

Created by Connor Dye as an California Polytechnic University Project.

Features
--------

* Basic commands: `exit`, `pwd`, `clear` and `cd` are supported
* Program invocation with forking and child processes
* supports I/O redirection (uses `dup2` system call to modify file descriptors)
* Pipelining implemented (`<cmd1> | <cmd2>`) via `pipe` and `dup2` syscalls. Multiple pipelining is supported.
* SIGINT signal handling is supported when Ctrl-C is pressed (shell is not exited)
* libmush.a handles the parsing in the shell

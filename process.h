#ifndef PROCESSH
#define PROCESS

void built_in_command(pipeline pipeline, int verbose);
void fork_and_execute(pipeline pipeline, int verbose);
void child_process(pipeline pipeline, int verbose);
void parent_process(pipeline pipeline, int verbose);
void single_process(pipeline pipeline, int verbose);
void IO_redirection(pipeline pipeline, int verbose);
#endif

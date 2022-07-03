#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <pwd.h>
#include "mush.h"
#include "process.h"
#include <signal.h>

/*VERBOSE IS FOR DEBUGGING */
#define verbose 0
#define MAX_LEN 1000

void sig_handler(int signum){
    char newline = '\n';
    char *buff = &newline;
    if(-1 == write(STDIN_FILENO, buff , 1) ){
        perror("write()");
    }
    /*empty to prevent ctrl c from closing terminal */
}

int main(int argc, char *argv[]){
    /*defined for parsing */
    FILE *file_in = stdin;
    FILE *file_out = stdout;
    int tty_stdin = 0; /*to check if the terminal is interactive */
    int tty_stdout = 0;
    char *commands = NULL; /*defined for char *readLongString(...) */
    pipeline pipeline;

    /*set up signal handler */
    struct sigaction sa;
    sa.sa_handler = &sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    /*SUPPORT FOR BATCH PROCESSING IF MUSH2 IS RUN WITH AN ARGUMENT */
    if(argc == 2){
        file_in = fopen(argv[1], "r");
    }
    if(argc != 1 && argc !=2){
        fprintf(stderr, "USAGE: ./mush2 [file]\n");
        exit(EXIT_FAILURE);
    }
    
    tty_stdin = isatty(STDIN_FILENO);
    tty_stdout = isatty(STDOUT_FILENO);
    if(tty_stdin != 0 && tty_stdin != 1 && tty_stdout != 0 && tty_stdout != 1){
        perror("isatty");
    }
    
    
        
    int done = 0;
    do{
        if(tty_stdin && tty_stdout && file_in == stdin){
            printf("8-P ");
        }
        fflush(stdout);
        
        /*parse commands*/
        if( (commands = readLongString(file_in)) == NULL){
            if(feof(file_in)){
                if(file_in == stdin){
                    printf("^D\n");
                }
                exit(EXIT_SUCCESS);
            }
            fprintf(stderr, "readLongString() failed to parse\n");
            continue;
        }

        /*pipeline contains struct clstage which has broken out arguments
         * for the stage as well as filenames for redirection of the input or
          output of the stage 
         * IF THESE ARE NULL, then stage expects its input/output (as 
            appropriate) to be from stdin and stdout
         * IF it's a filename, the input or output should be that file
         *crack_pipeline() seems to check if no arguments are given
         and issues an error */

        if( (pipeline = crack_pipeline(commands)) == NULL){
            fprintf(stderr, "crack_pipeline() failed to parse line\n");
            continue;
        }else if(verbose){
            print_pipeline(stdout, pipeline);
        }
        
        /*check to see if argv is a parent function (e.g. cd command)
         *cd takes either zero or one argument
         *if argument is absent, change to home directory
         *if argument is there, change to that directory
         */

        if( !strcmp(pipeline->stage->argv[0], "cd") ){
            built_in_command(pipeline, verbose); /*e.g command like cd */
        }
        else if(pipeline->length == 1 && (pipeline->stage->inname == NULL 
                && pipeline->stage->outname == NULL)){
            single_process(pipeline, verbose); /*single command, no redirection*/
        }
        else if(pipeline->length > 1){
            fork_and_execute(pipeline, verbose); /*pipeline*/
        }
        else{
            IO_redirection(pipeline, verbose);
        }

    }while(!done);
    
    return 0;
}

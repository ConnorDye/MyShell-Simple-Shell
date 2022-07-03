#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <pwd.h>
#include <sys/wait.h>
#include "mush.h"
#include "process.h"
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

void single_process(pipeline pipeline, int verbose);

#define WRITE 1
#define READ 0

/*main checks to see if argv is a parent function (e.g. cd command)
 *cd takes either zero or one argument
 *if argument is absent, change to home directory
 *if argument is there, change to that directory
 */
void built_in_command(pipeline pipeline, int verbose){
        if( !strcmp(pipeline->stage->argv[0],"cd") && 
                            pipeline->stage->argv[1] == NULL){
            /*determine home directory uses environment variable first,
            and if that fails then it uses pwuid*/
        
            if(-1 == chdir(getenv("HOME"))){
                perror("chdir");

                uid_t user_id = getuid();
                struct passwd *user = getpwuid(user_id);
                if(-1 == chdir(user->pw_dir)){
                    fprintf(stderr, "unable to determine home directory\n");
                }
            }
            if(verbose){
                char cwd[1024];
                getcwd(cwd, sizeof(cwd));
                printf("changed directory to %s\n", cwd);
            }
        }else if( !strcmp(pipeline->stage->argv[0],"cd") && 
                            pipeline->stage->argv[1] != NULL){
            /*if path given, go there stored in argv[1] of pipeline */
            if(-1 == chdir(pipeline->stage->argv[1])){
                perror("chdir");
            }
            if(verbose){
                char cwd[1024];
                getcwd(cwd, sizeof(cwd));
                printf("changed directory to %s\n", cwd);
            }
        }
}

/*if not a built in function, we must fork and execute a child process 
    *must handle I/O redirection
    *must handle pipes "|" - i.e ls | sort makes the output of ls the input of
    sort. A series of commands seperated by pipes is a pipeline
    *support for "\" escapes */

/*DETAILS:
    *number of commands given by pipeline->length
    *
    */
void fork_and_execute(pipeline pipeline, int verbose){
    pid_t pid; /*declared to differentiate parent from child */
    int p_one[2]; /*create pipes */
    int p_two[2];
    
    for(int i = 0; i < pipeline->length; i++){
        /*depending on what iteration we are in, we will set different
        descriptors for the pipes inputs and output. This way, a pipe will be
        shared between two iterations, enabling us to connect the inputs
        and outputs of the two different commands */
        if(i % 2 != 0){ //for odd i
            if(pipe(p_one) == -1){ 
                perror("pipe"); 
                exit(EXIT_FAILURE);
            }
        }else{          //for even i
            if(pipe(p_two) == -1){ 
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid = fork();
        if(-1 == pid){
            perror("fork error in fork_and_execute()... " 
                                        "failed to complete command\n");
            /*should i close file descriptors?*/
            return;
        }
        /*CHILD PROCESS*/
        if(pid == 0){
            /*if we are in first command, write end of pipe goes to 
            next child 
            *we also need to check to see if we need to take input
            from stdin */
            if(i == 0){ 
                if( -1 == dup2(p_two[WRITE], STDOUT_FILENO) ){
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                /*ONLY CAN DUP IN AT BEGINNING OF PIPE */
                if(pipeline->stage[0].inname != NULL){
                    int fileDesc;
                    if( (fileDesc = open(pipeline->stage[0].inname, 
                                                        O_RDONLY)) == -1){
                        perror("open");
                        return;
                    }
                    if( -1 == dup2(fileDesc, STDIN_FILENO) ){
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            /*if we are at the end of the path, depending on whether
            it's in an odd or even position, we replace the stdin for one
            pipe or another. Standard output remains the same as it goes to
            the terminal */
            else if( i == pipeline->length - 1 ){
                if(pipeline->length % 2 != 0){ //odd # commands
                    if( -1 == dup2(p_one[READ], STDIN_FILENO) ){
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }
                else{                          //even # commands
                    if( -1 == dup2(p_two[READ], STDIN_FILENO) ){
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }
                /*IF WE REDIRECT I/O TO FILE THEN WE NEED TO DUP */
                if(pipeline->stage[pipeline->length -1].outname != NULL){
                    int fileDesc;
                    if( (fileDesc = open(pipeline->stage[pipeline->length-1].outname, 
                        O_CREAT | O_TRUNC | O_WRONLY, 0666)) == -1){
                        perror("open");
                        return;
                    }
                    if( -1 == dup2(fileDesc, STDOUT_FILENO) ){
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            /*if we are in a command in the middle, there will be two
            pipes open, one for input and another for output. 
            The position (whether it is odd or even) is also important to
            choose which file descriptor corresponds to each input/output*/
            else{
                if(i % 2 != 0){ //for odd i
                    if( -1 == dup2(p_two[READ], STDIN_FILENO) ){
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    if( -1 == dup2(p_one[WRITE], STDOUT_FILENO) ){
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }else{
                    if( -1 == dup2(p_one[READ], STDIN_FILENO) ){
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    if( -1 == dup2(p_two[WRITE], STDOUT_FILENO) ){
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            
           if (execvp(pipeline->stage[i].argv[0], pipeline->stage[i].argv)
                            == -1){
                fprintf(stderr, "failed to execute command in child\n");
                exit(0); /*we need to make sure the child process terminates*/
            }

        /*PARENT PROCESS */
        }else{
            /*CLOSING DESCRIPTORS IN PARENT */
            if(i == 0){ /*if we're on the first iteration */
                close(p_two[WRITE]);
            }
            /*so lets say we're at the end... we have two pipes open
            so we have to close one of them so we know where to read from and
            it will go to standard out */
            else if(i == pipeline->length -1){
                if(pipeline->length % 2 != 0){
                    close(p_one[READ]);
                }else{
                    close(p_two[READ]);
                }
                
            }
            else{
                if( i % 2 != 0){
                    close(p_two[READ]);
                    close(p_one[WRITE]);
                }else{
                    close(p_one[READ]);
                    close(p_two[WRITE]);
                }
            }
           
           /*
           int status;
           if(-1 ==  waitpid(pid, &status, 0)){
                perror("waitpid");
                exit(EXIT_FAILURE);
           }

           if(WIFEXITED(status)){
                if(verbose){
                    printf("Child exited with status %d\n",WEXITSTATUS(status));
                }
           }
           if(WIFSIGNALED(status)){
                printf("Child exited via signal %d\n", WTERMSIG(status));
           } */

        }
    }
    int wpid;
    int status;
    while ( ((wpid = wait(&status)) > 0) || errno == EINTR);
    fflush(stdout);
}

/*Defined for no pipes */
void single_process(pipeline pipeline, int verbose){
    pid_t pid;
    
    pid = fork();

    if(pid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(pid == 0){
       if (execvp(pipeline->stage->argv[0], pipeline->stage->argv)
                        == -1){
            perror(pipeline->stage->argv[0]);
            fprintf(stderr, "failed to execute command in child\n");
            exit(0); /*we need to make sure the child process terminates*/
        }
    }else{
        int status; /*defined for wait */
        int wpid = 0;
        /*wait for the child*/ 
        while ((wpid = wait(&status)) > 0 || errno == EINTR);
    }
}

/* FUNCTIONALITY of IO_redirection()
    *3 options: output redirection, input redirection, or 
    input and output redirection 
    *
    *
    *
*/
void IO_redirection(pipeline pipeline, int verbose){
    char *inputFile = pipeline->stage->inname;
    char *outputFile = pipeline->stage->outname;
    int pid; 
    int fileDesc;

    if(verbose){
        printf("IO_redirection entered\n");
    }

    pid = fork();
    if(pid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(pid == 0){
        /*Open files*/
        if(inputFile != NULL){
            if( (fileDesc = open(inputFile, O_RDONLY)) == -1){
                perror("open");
                return;
            }
            if( -1 == dup2(fileDesc, STDIN_FILENO) ){
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            if(verbose){
                printf("input file created in IO_redirection()\n");
            }
            close(fileDesc);
            
        }
        if(outputFile != NULL){
            if( (fileDesc = open(outputFile, O_CREAT | O_TRUNC 
                        | O_WRONLY, 0666)) == -1){
                perror("open");
                return;
            }
            if( -1 == dup2(fileDesc, STDOUT_FILENO) ){
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            if(verbose){
                printf("output file created in IO_redirection()\n");
            }
            close(fileDesc);
        }
        /*execute command with IO redirection */
        if (execvp(pipeline->stage->argv[0], pipeline->stage->argv) == -1){
                fprintf(stderr, "failed to execute command in child\n");
                exit(0); /*we need to make sure the child process terminates*/
        }
    }else{
       int status;
       if(-1 ==  waitpid(pid, &status, 0)){
            perror("waitpid");
            exit(EXIT_FAILURE);
       }

       if(WIFEXITED(status)){
            if(verbose){
                printf("Child exited with status %d\n",WEXITSTATUS(status));
            }
       }
       if(WIFSIGNALED(status)){
            printf("Child exited via signal %d\n", WTERMSIG(status));
       } 
    }

}

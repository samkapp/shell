/**
 * Implementation File for commands
 * 
 * Deals with cd, and exit commands 
 * Along with all simple commands
 * Can handle redirection 
 * Can handle a single pipe
 * 
 * NOTE: chatgpt was used to reset file descriptors in redirection, along
 *          with refactoring mode to be set through the if statements
 * 
 * @author Sam Kapp
*/
#include "commands.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>

/**
 * Takes argc and argv and runs the appropriate command for it
*/
void parse(int argc, char *argv[]) {
    // Check for which command to run
    if (strcmp(argv[0], "exit") == 0) {
        exit_cmd(argc, argv);
    } else if (strcmp(argv[0], "cd") == 0) {
        cd_cmd(argc, argv);
    } else if (redirection_cmd(argc, argv) != 0) {
        return;
    } else if (pipe_cmd(argc, argv) != 0) {
        return; 
    } else {
        simple_cmd(argc, argv);
    }

}

/**
 * Executes the exit command in the shell
*/
void exit_cmd(int argc, char *argv[]) {
    if (argc != 1) {
        printf("exit: too many arguments.\n");
        return;
    } else {
        printf("Exiting Shell.\n");
        exit(0);
        printf("shoulnd't reach here.\n");
    }
}

/**
 * Executes the cd command in the shell
*/
void cd_cmd(int argc, char *argv[]) {
    if (argc > 2) {
        printf("cd: too many arguments.\n");
        return;
    } else {
        char *dir = NULL; 
        if (argc == 1 || strcmp(argv[1], "~") == 0) {
            dir = getenv("HOME");
        } else {
            dir = argv[1];
        }

        if (chdir(dir) != 0) {
            printf("chdir() error.\n");
        }
    }
}

/**
 * Executes any simple command
 * 
 * Shell first forks a child
 * The child process will call execvp and replace its space 
 *      with the disk image of the given command 
 * The child will then run the command
*/
void simple_cmd(int argc, char *argv[]) {
    // Check for '&'
    bool run_background = false; 
    if (strcmp(argv[argc-1], "&") == 0) {
        run_background = true; 
        argv[argc-1] = NULL;
    }

    // Fork the process
    pid_t pid = fork(); 

    if (pid == -1) {
        printf("fork() error.\n");
        return;
    } else if (pid == 0) { 
        // Child 
        if (execvp(argv[0], argv) == -1) {
            printf("%s: command not found.\n", argv[0]);
            return;
        }
    } else  {
        // Parent
        if (run_background) {
            printf("Background Process: %d\n", pid);\
        } else {
            wait(NULL);
        }
    }
}

/**
 * Checks for redirection 
 * If present, will set the file descriptors 
 * to their appropriate files.
 * 
 * If redirection is present, recall parse() 
 *      to execute the command with the 
 *      newly set file descriptors
*/
int redirection_cmd(int argc, char *argv[]) {
    // Status of redirection. 0 is none found. 1 is found and executed. -1 is error
    int status = 0;
    // Check for redirection
    // Save stdin and stdout to reset file descriptors at the end 
    int original_stdin = dup(0); 
    int original_stdout = dup(1);

    // Create new args without redirection symbols present
    int new_argc = 0;
    char *new_argv[argc];

    

    // Loop through argv looking for redirection
    for (int i = 0; i < argc; i++) {
        // Symbol Found
        if (strcmp(argv[i], "<") == 0 || 
            strcmp(argv[i], ">") == 0 || 
            strcmp(argv[i], ">>") == 0) {
                // Make sure a file was give
                if (argv[i+1] != NULL) {
                    // keep track of what modes are needed
                    int mode; 

                    // stdin 
                    if (strcmp(argv[i], "<") == 0) {
                        mode = O_RDONLY;
                    // stdout that overwrite 
                    } else if (strcmp(argv[i], ">") == 0) {
                        mode = O_CREAT | O_WRONLY | O_TRUNC;
                    // stdout that doesn't overwrite
                    } else {
                        mode = O_CREAT | O_WRONLY | O_APPEND;
                    }

                    int fd = open(argv[i+1], mode, 0666);
                    if (fd == -1) {
                        printf("error opening file.\n");
                        return -1;
                    }

                    // Overwrite the appropriate file descriptor tables
                    if (strcmp(argv[i], "<") == 0) {
                        close(0); 
                        int dup1 = dup(fd);
                        if (dup1 < 0) {
                            printf("error with dup1.\n");
                            return -1;
                        }
                        close(fd);
                        status = 0;
                    } else {
                        close(1); 
                        int dup2 = dup(fd); 
                        if (dup2 < 0) {
                            printf("error with dup2.\n");
                            return -1;
                        }
                        close(fd);
                    }
                // Move past redirection symbol and filename
                status = 1;
                i++;
                } else {
                    printf("no file given for redirection.\n"); 
                    return -1;
                }
        } else {
            // No symbol found, and not a filename so add to new_argv
            // and increment new_argc
            new_argv[new_argc++] = argv[i];
        }
    }

    // Redirection found, re send new argc and argv to 
    // parse, to run the command with the file descriptors 
    // set to their new values
    if (status == 1) {
        new_argv[new_argc] = NULL;
        parse(new_argc, new_argv);
    }

    // return file descriptor table back to normal
    dup2(original_stdin, 0);
    dup2(original_stdout, 1);
    close(original_stdin);
    close(original_stdout);

    return status;
}

/**
 * Checks for a pipe symbol
 * If found, splits argv into the two arguments, sets up the pipe
 *  and then executes the arguments
*/
int pipe_cmd(int argc, char *argv[]) {
    // Status of redirection. 0 is none found. 1 is found and executed. -1 is error
    int status = 0; 

    // Loop through and check for '|'
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "|") == 0) {
            status = 1;
        }
    }

    // If pipe found, split up argv
    if (status == 1) {
        int argc1 = 0;
        int argc2 = 0;
        char *argv1[argc]; 
        char *argv2[argc];

        // bool to split argv between pipe
        bool pipe_found = false; 

        // Fill up argv1 and argv2
        for (int i = 0; i < argc; i++) {
            if (strcmp(argv[i], "|") == 0) {
                pipe_found = true;
                continue;
            }

            // add proper args to argv1 and argv2
            if (!pipe_found) {
                argv1[argc1++] = argv[i];
            } else {
                argv2[argc2++] = argv[i];
            }
        }

        // Add null terminators 
        argv1[argc1] = NULL;
        argv2[argc2] = NULL;
        
        // Check for '&'
                bool run_background1 = false; 
                bool run_background2 = false; 
                if (strcmp(argv1[argc1-1], "&") == 0) {
                    run_background1 = true; 
                    argv1[argc1-1] = NULL;
                }
                if (strcmp(argv2[argc2-1], "&")  == 0) {
                    run_background2 = true; 
                    argv2[argc2-1] = NULL;
                }

        // Sets up the file descriptors and executes the command
        
        // Contains two ints to hold file descriptors for input and output
        int fd[2];

        // Attempt to execute pipe
        if (pipe(fd) == -1) {
            printf("pipe(fd) error.\n");
            return -1;
        } else {
            pid_t pid1 = fork(); 
            if (pid1 == -1) {
                printf("fork() error.\n");
                return -1;
            // Child process for the first command
            } else if (pid1 == 0) {
                // Close input end of pipe
                close(fd[0]);
                // Redirecet stdout to write end of pipe
                dup2(fd[1], 1);
                // Close write end of pipe
                close(fd[1]);
                // Execute first command
                execvp(argv1[0], argv1);
            // Parent Process
            } else {
                pid_t pid2 = fork(); 
                if (pid2 == -1) {
                    printf("fork() error.\n");
                    return -1;
                // Child process for second command
                } else if (pid2 == 0) {
                    // Close write end of pipe
                    close(fd[1]);
                    // redirect stdin to input end of pipe
                    dup2(fd[0], 0);
                    // Close input end of pipe
                    close(fd[0]);
                    // Execute the second command
                    execvp(argv2[0], argv2);
                }

                // Close unused ends of pipes 
                close(fd[0]); 
                close(fd[1]);



                if (!run_background1) {
                    wait(NULL);
                } 
                if (!run_background2) {
                    wait(NULL);
                }
                return status;
            }
        }
    } 

    return 0;
}
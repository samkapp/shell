/**
 * Header file for commands 
 * 
 * @author Sam Kapp
*/
#ifndef COMMANDS_H
#define COMMANDS_H

void parse(int argc, char *argv[]);
void exit_cmd(int argc, char *argv[]);
void cd_cmd(int argc, char *argv[]);
void simple_cmd(int argc, char *argv[]);
int redirection_cmd(int argc, char *argv[]);
int pipe_cmd(int argc, char *argv[]);

#endif
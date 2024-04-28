/**
 * Main file for UNIX Shell
 *      Deals with the startup, display screen, and main loop
 * 
 * 
 * TODO: 
 *  autocomplete
 *  
 * Note: exit command just sometimes doesn't work idk why
 *          if needed 'ctrl+z' to pause and then 'kill KILL %%' will
 *          terminate it
 * 
 * @author Sam Kapp
*/
#include "commands.h" 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <time.h> 
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdbool.h>
#include <termios.h>
#include <signal.h>

void display();
void sig_handler(int signo);

#define max(x, y) (((x) > (y)) ? (x) : (y))

// Functions for different modes
// Needed to deal with key_presses in interactive mode
void interactive_mode();
void batch_mode();
FILE *input_stream = NULL;

// Key press handler functions/struct
static struct termios old, current;
void initTermios(int echo);
void resetTermios(void);
char getch_(int echo);
char getch();
char getche();

// History functions / globals
char *history[100];
int history_size = 0;
int history_index;
void display_history();


int main(int s_argc, char *s_argv[]) {

    // check for SIGINT signal (user wants to use autocomplete feature)
    signal(SIGINT, sig_handler);

    display();

    // Check if batch mode 
    bool batch = false; 
    FILE *batch_file = NULL;
    input_stream = stdin;
    if (s_argc > 1) {
        batch = true;
        batch_file = fopen(s_argv[1], "r");
        if (!batch_file) {
            printf("Error: unable to open batch file.\n");
            return -1;
        }
        input_stream = batch_file;
    }

    // If batch file is there run it in batch mode
    if (batch) {
        batch_mode();
    } else {
        interactive_mode();
    }

    // After running file in batch mode switch to interactive mode
    if (batch) {
        interactive_mode();
    }

    // Close the batch file if in batch mode
    if (batch) {
        fclose(batch_file);
    }

    return 0;
}

// Interactive mode function
void interactive_mode() {
    // MAIN LOOP
    while (1) {
        // User cursor location
        printf("> ");

        // Initialize user_input buffer
        char user_input[1000]; // Assuming a maximum input length of 1000 characters
        int input_length = 0;

        // Inner loop for reading characters until Enter is pressed
        char key_press;
        while ((key_press = getch()) != '\n') {
            // Handle arrow keys and ctrl-c
            switch (key_press) {
                // ESC, which is the beginning of an escape sequence for arrow keys
                case 27: 
                    // Read the next two characters to identify the arrow key
                    if (getch() == '[') {
                        char arrow_key = getch();
                            switch (arrow_key) {
                                case 'A': // Up Arrow
                                    if (history_index > 0) {
                                        // Clear the entire line
                                        int max_length = max(strlen(history[history_index]), input_length);
                                        for (int i = 0; i < max_length + 2; i++) {
                                            printf("\b \b"); // Print backspace and space to clear
                                        }
                                        fflush(stdout); // Flush output buffer to ensure changes are immediately visible
                                        printf("> ");
                                        printf("%s", history[history_index]);
                                        // Update user_input and input_length to match the history command
                                        strcpy(user_input, history[history_index]);
                                        input_length = strlen(user_input);
                                        history_index--; // Move backward through the history
                                    }
                                    break;
                                case 'B': // Down Arrow
                                    if (history_index < history_size) {
                                        // Clear the entire line
                                        int max_length = max(strlen(history[history_index + 1]), input_length);
                                        for (int i = 0; i < max_length + 2; i++) {
                                            printf("\b \b"); // Print backspace and space to clear
                                        }
                                        fflush(stdout); // Flush output buffer to ensure changes are immediately visible
                                        printf("> ");
                                        history_index++; // Move forward through the history
                                        printf("%s", history[history_index]);
                                        // Update user_input and input_length to match the history command
                                        strcpy(user_input, history[history_index]);
                                        input_length = strlen(user_input);
                                    }
                                    break;
                            }
                    }
                    break;
                case 127: // Backspace
                    if (input_length > 0) {
                        printf("\b \b"); // Move cursor back and erase character
                        user_input[--input_length] = '\0'; // Remove the last character from user_input
                    }
                    break;
                default:
                    printf("%c", key_press);
                    user_input[input_length++] = key_press; // Append the character to user_input
                    break;
            }
        }
        printf("\n");

        // Null-terminate the user_input string
        user_input[input_length] = '\0';

        // Parse user_input if it's not empty
        if (strlen(user_input) > 0) {
            // Put the command into history
            history[++history_size] = strdup(user_input);
            if (history[history_size] == NULL) {
                printf("Error: Memory allocation failed for history entry.\n");
            } else {
                history_index = history_size;
            }

            // Command read properly parse into argc and argv
            int argc = 0;
            char **argv = malloc(sizeof(char *) * (input_length + 1)); // +1 for NULL terminator
            if (argv == NULL) {
                printf("Memory allocation failed.\n");
            } else {
                // Tokenize user_input to get individual words 
                char *token = strtok(user_input, " \n\r\t");
                while (token != NULL) {
                    // Copy token into argv and allocate memory for it
                    argv[argc++] = strdup(token);
                    token = strtok(NULL, " \n\r\t");
                }

                // Add NULL to end of argv 
                argv[argc] = NULL;

                // Check if history command, if so print it out here, else send to parse
                if (argc >= 1 && strcmp(argv[0], "history") == 0) {
                    display_history();
                } else {
                    // Send argc and argv to be parsed
                    parse(argc, argv);
                }

                // Free memory allocated for each token
                for (int i = 0; i < argc; ++i) {
                    free(argv[i]);
                }
                free(argv);
            }
        }
    }
}


void batch_mode() {
    // Read commands from the batch file
    char *user_input = NULL; 
    size_t input_size = 0;

    while (getline(&user_input, &input_size, input_stream) != -1) {
        // Remove trailing newline character
        if (user_input[strlen(user_input) - 1] == '\n') {
            user_input[strlen(user_input) - 1] = '\0';
        }

        // Put the command into history
        history[++history_size] = strdup(user_input);
        if (history[history_size] == NULL) {
            printf("Error: Memory allocation failed for history entry.\n");
        } else {
            history_index = history_size;
        }

        // Command read properly parse into argc and argv
        int argc = 0;
        char **argv = malloc(sizeof(char *) * input_size);
        if (argv == NULL) {
            printf("Memory allocation failed.\n");
            continue; // Skip the rest of the loop iteration
        }

        // Tokenize user_input to get individual words 
        char *token = strtok(user_input, " \n\r\t");
        while (token != NULL) {
            // Copy token into argv and allocate memory for it
            argv[argc++] = strdup(token);
            token = strtok(NULL, " \n\r\t");
        }

        // Add NULL to end of argv 
        argv[argc] = NULL;

        // Check if history command, if so print it out here, else send to parse
        if (argc >= 1 && strcmp(argv[0], "history") == 0) {
            display_history();
        } else {
            // Send argc and argv to be parsed
            parse(argc, argv);
        }

        // Free memory allocated for each token
        for (int i = 0; i < argc; ++i) {
            free(argv[i]);
        }
        free(argv);
    }

    // Free memory allocated for getline
    free(user_input);
}

/**
 * Displays the startup ascii image 
 * along with welcome message
*/
void display() {
    // Display the ascii art 
    FILE *file = fopen("ascii.txt", "r");
    if (file == NULL) {
        printf("Error: Failed to open 'ascii.txt'.\n");
    } else {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            if (line[0] == '#') {
                // Skip any comments 
                continue; 
            } else {
                printf("%s", line);
            }
        }
        fclose(file);
    }

    // Display the welcome message 
    printf("\nWelcome to Shell-Hulud!\n");

    // Display date & time in the format: weekday month day time year
    time_t t = time(NULL);
     struct tm *tm = localtime(&t);
    printf("%s\n", asctime(tm));

    printf("Created by Sam Kapp.\n");
}

void display_history() {
    for (int i = 1; i < history_size+1; i++) {
        printf("%d: %s\n", i, history[i]);
    }
}

void sig_handler(int signo) {
    exit(0);
}


/**
 * All code/functions below are for reading user input
 * Not mine, gotten from https://stackoverflow.com/questions/7469139/what-is-the-equivalent-to-getch-getche-in-linux
*/
/* Initialize new terminal i/o settings */
void initTermios(int echo) 
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  current = old; /* make new settings same as old settings */
  current.c_lflag &= ~ICANON; /* disable buffered i/o */
  if (echo) {
      current.c_lflag |= ECHO; /* set echo mode */
  } else {
      current.c_lflag &= ~ECHO; /* set no echo mode */
  }
  tcsetattr(0, TCSANOW, &current); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void) 
{
  return getch_(0);
}

/* Read 1 character with echo */
char getche(void) 
{
  return getch_(1);
}

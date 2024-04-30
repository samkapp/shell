/**
 * Main file for UNIX Shell
 * 
 * Shell has two modes
 *  Interactive Mode:
 *      Interactive mode is for users to execute their commands
 *  Batch Mode: 
 *      Batch mode is solely for the execution of batch files
 * 
 * Shell can execute any basic commands, along with cd and exit
 * 
 * Shell also keeps track of the users command history and allows them 
 *      to arrow key through the history list 
 * 
 * Shell has an autocomplete feature, which is turned on/off with ctrl+c
 *  
 * Note: exit command just sometimes doesn't work and i'm not sure why
 *          sometimes just running the command again will get it to work
 *          if not ctrl+z and kill KILL %% will terminate it 
 *          since ctrl+c is no longer used for that
 * 
 * Note: For autocomplete, since it is supposed to autocomplete when 
 *          there is only one match in history, if there are multiple of 
 *          the same command in the history (ex ["ls", "ls"]), it won't 
 *          autocomplete for it. 
 * 
 * Note: ChatGPT helped me with the implementation of key_presses although 
 *          most of the logic is my own, chatGPT helped with the code
 * 
 * Note: With the way user_input is obtained you won't be able to delete
 *          characters in certain occasions.
 * 
 * Code for getting key press from: https://stackoverflow.com/questions/7469139/what-is-the-equivalent-to-getch-getche-in-linux
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

// Different Mode prototypes and variable
void interactive_mode();
void batch_mode();
FILE *batch_file = NULL;

// Key press functions / struct / prototypes
static struct termios old, current;
void initTermios(int echo);
void resetTermios(void);
char getch_(int echo);
char getch();
char getche();

// History variables and prototypes
char *history[10000];
int history_size = -1; 
int history_index;
void display_history();
int max(int x, int y);

// Global boolean for autocomplete 
bool is_auto = false;

int main(int s_argc, char *s_argv[]) {
    // check for SIGINT signal (user wants to use autocomplete feature)
    signal(SIGINT, sig_handler);

    display();

    // Check if batch mode 
    bool batch = false; 
    if (s_argc > 1) {
        // Only accept one batch file argument
        if (s_argc == 2) {
            batch = true;
            batch_file = fopen(s_argv[1], "r");
            if (!batch_file) {
                printf("Error: unable to open batch file.\n");
                return -1;
            }
        } else {
            printf("Error: too many files provided.\n");
        }
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

/**
 * Interactive mode is the base mode for the shell
 * where the user is able to directly interact with the shell
 */
void interactive_mode() {
    // MAIN LOOP
    while (1) {
        // User cursor location
        // Sky Blue ANSI escape sequence
        printf("\033[38;5;39m");
        printf("> ");
        printf("\033[0m");

        // Initialize user_input buffer
        char user_input[10000];
        int input_length = 0;

        // Get user input through getch function
        char key_press;
        while ((key_press = getch()) != '\n') {
            // Look for Arrow Keys  (Handling History scroll through)
            // Backspace to delete user input
            // Default will print char to screen, and possibly autocomplete
            switch (key_press) {
                // ESC, which is the beginning of an escape sequence for arrow keys
                case 27: 
                    // Read Arrow Keys ([A for up and [B for down)
                    if (getch() == '[') {
                        char arrow_key = getch();
                            switch (arrow_key) {
                                case 'A': // Up Arrow
                                    if (history_index > 0) {
                                        // Clear the line
                                        int max_length = max(strlen(history[history_index]), input_length);
                                        for (int i = 0; i < max_length + 2; i++) {
                                            printf("\b \b"); 
                                        }
                                        fflush(stdout); 
                                        printf("\033[38;5;39m");
                                        printf("> ");
                                        printf("\033[0m");
                                        // Print out the correct command from history
                                        printf("%s", history[history_index]);
                                        // Update user_input and input_length to match the history command
                                        strcpy(user_input, history[history_index]);
                                        input_length = strlen(user_input);
                                        history_index--; 
                                    }
                                    break;
                                case 'B': // Down Arrow
                                    if (history_index < history_size) {
                                        // Clear the entire line
                                        int max_length = max(strlen(history[history_index + 1]), input_length);
                                        // Clear the entire line
                                        for (int i = 0; i < max_length + 2; i++) {
                                            printf("\b \b"); 
                                        }
                                        fflush(stdout); 
                                        printf("\033[38;5;39m");
                                        printf("> ");
                                        printf("\033[0m");
                                        history_index++;
                                        // Print out the correct command from history
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
                        // Clear line by moving cursor back once
                        // printing a space instead of the char there
                        // moving the cursor back once again
                        printf("\b \b"); 
                        // Remove the last character from user_input
                        user_input[--input_length] = '\0'; 
                    }
                    break;
                default:
                    if (is_auto) {
                        printf("%c", key_press);
                        user_input[input_length++] = key_press;

                        if (input_length > 0) {
                            // keep track of number of matches
                            // and the index of the match
                            int match_count = 0;
                            int match_index = -1;

                            // Loop through all of hiustory
                            for (int i = 0; i < history_size + 1; i++) {
                                bool is_match = true;
                                // Compare current history and user_input char by char
                                for (int j = 0; user_input[j] != '\0'; j++) {
                                    if (history[i][j] != user_input[j]) {
                                        is_match = false;
                                        break;
                                    }
                                }
                                if (is_match) {
                                    match_count++;
                                    match_index = i;
                                }
                            }
                            // Only fill in if there's exactly one match
                            if (match_count == 1) {
                                // Clear the line
                                for (int i = 0; i < input_length; i++) {
                                    printf("\b \b");
                                }
                                printf("%s", history[match_index]);
                                strcpy(user_input, history[match_index]);
                                input_length = strlen(user_input);
                            } else {
                                match_count = 0;
                            }
                        }
                    } else {
                        printf("%c", key_press);
                        user_input[input_length++] = key_press;
                    }

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
            char **argv = malloc(sizeof(char *) * (input_length + 1));
            if (argv == NULL) {
                printf("Memory allocation failed.\n");
            } else {
                // Tokenize user_input to get individual words 
                char *token = strtok(user_input, " \n\r\t");
                while (token != NULL) {
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

/**
 * Batch mode for dealing with batch files 
 * Batch files are only gotten through calling the startup of calling the shell
 *  ex: ./shell files.bat
 * 
 * Executes the batch files and when comeplete returns the user to interactive mode
*/
void batch_mode() {
    // Read commands from the batch file
    char *user_input = NULL; 
    size_t input_size = 0;

    while (getline(&user_input, &input_size, batch_file) != -1) {
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
        if (argv == NULL) {                                printf("> ");
                                        printf("\033[0m");
            printf("Memory allocation failed.\n");
            continue; 
        }

        // Tokenize user_input to get individual words 
        char *token = strtok(user_input, " \n\r\t");
        while (token != NULL) {
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
    printf("\033[38;5;208m"); 
    // Reddish-Orange ANSI escape sequence
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
    printf("\033[0m"); 

    printf("\033[38;5;39m");
    printf("> ");
    
    // Display the welcome message 
    printf("\nWelcome to Shell-Hulud!\n");

    // Display date & time in the format: weekday month day time year
    time_t t = time(NULL);
     struct tm *tm = localtime(&t);
    printf("%s", asctime(tm));

    srand(time(NULL));
    // Number between 50 and 70
    int r = 50 + rand() % 20;
    printf("Current Temparature: %d°Celsius\n", r); 
    printf("Created by Sam Kapp.\n");
    printf("\033[0m");
}

/**
 * Displays the users history command
*/
void display_history() {
    for (int i = 0; i < history_size+1; i++) { // Changed loop initialization
        printf("%d: %s\n", i + 1, history[i]); // Adjusted index to start from 1
    }
}

/**
 * Simple function to return the max value of two integers
*/
int max(int x, int y) {
    return (x > y) ? x : y;
}


/**
 * sig handler for SIGINT (ctrl+c)
 * Handles the autocomplete feature
*/
void sig_handler(int signo) {
    is_auto = !is_auto;
}

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

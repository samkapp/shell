/**
 * Main file for UNIX Shell
 *      Deals with the startup, display screen, and main loop
 * 
 * 
 * TODO: 
 *  Command History
 *  
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
#include <stdbool.h>

void display();


int main(int s_argc, char *s_argv[]) {
    display();

    // Check if batch mode 
    bool batch_mode = false; 
    FILE *input_stream = stdin;
    FILE *batch_file = NULL;
    if (s_argc > 1) {
        batch_mode = true;
        batch_file = fopen(s_argv[1], "r");
        if (!batch_file) {
            printf("Error: unable to open batch file.\n");
            return -1;
        }
        input_stream = batch_file;
    }

    // MAIN LOOP
    while (1) {
        // User cursor location, only shows if not in batch mode
        if (!batch_mode) {
            printf("> ");
        }

        char *user_input = NULL; 
        size_t input_size = 0;

        if (getline(&user_input, &input_size, input_stream) == -1) {
            // Check if in batch mode, reset to interactive mode if so
            if (batch_mode) {
                batch_mode = false; 
                input_stream = stdin; 
                continue;
            } else {
                printf("Error reading command. Please try again.\n");
                continue;
            }
        } else if (strcmp(user_input, "\n") == 0) {
            // Empty command entered, ignore and continue
        } else {
            // Command read properly parse into argc and argv
            int argc = 0;
            char **argv = malloc(sizeof(char *) * input_size);
            if (argv == NULL) {
                printf("Memory allocation failed.\n");
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

            // Send argc and argv to be parsed
            parse(argc, argv);
            // Free memory allocated for each token
            for (int i = 0; i < argc; ++i) {
                free(argv[i]);
            }
            free(argv);
        }
        free(user_input);
    } 

    // Close the batch file if in batch mode
    if (batch_mode) {
        fclose(batch_file);
    }

    return 0;
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
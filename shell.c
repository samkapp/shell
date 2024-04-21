/**
 * Main file for UNIX Shell
 *      Deals with the startup, display screen, and main loop
 * 
 * NOTE: stdin redirection '<' when using the shell as the 
 *          file to input into (ex: ./shell < input.txt)
 *          results in an infinite loop, after reading through 
 *          the file.
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

void display();

int main() {
    display();

    // MAIN LOOP
    while (1) {
        // User cursor location 
        printf("> ");

        char *user_input = NULL; 
        size_t input_size = 0;

        if (getline(&user_input, &input_size, stdin) == -1) {
            printf("Error reading command. Please try again.\n");
            continue;
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
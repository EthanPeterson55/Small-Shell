#include "shell.h"


void handle_bg_process_completion() {
    int i;
    for (i = 0; i < num_bg_processes; i++) {
        pid_t pid = bg_processes[i].pid;
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        } else if (result > 0) {
            // Background process has terminated
            bg_processes[i].exit_status = status;
            bg_processes[i].signaled = WIFSIGNALED(status);
            printf("Background process with PID %d ", pid);
            fflush(stdout);
            if (WIFEXITED(status)) {
                printf("exited with value %d.\n", WEXITSTATUS(status));
                fflush(stdout);
            }else if (WIFSIGNALED(status)) {
                printf("terminated due to signal %d.\n", WTERMSIG(status));
                fflush(stdout);
            }
            // Remove the completed background process from the array
            memmove(&bg_processes[i], &bg_processes[i + 1], (num_bg_processes - i - 1) * sizeof(BackgroundProcess));
            num_bg_processes--;
            i--; // Since we shifted elements left, recheck this index
        }
    }
}

void execute(char* array[], struct sigaction ignact, int argc) {
    fflush(stdout); // Flush stdout before forking

    int runInBackground = 0;
    if (strcmp(array[argc - 1], "&") == 0) {
        array[argc - 1] = NULL; // Remove the '&' from the argument list
        argc--; // Decrement argc since we removed one argument
        if(fgbool == 0){
            runInBackground = 1;
        }
    }

    pid_t childPid = fork(); // returns process ID of child process

    switch(childPid) {
        case -1:
            perror("fork failed"); // if fork fails
            exit(1);
            break;  
        case 0:

            if (runInBackground) { // if running in background command
                int devNull = open("/dev/null", O_RDWR);
                dup2(devNull, STDIN_FILENO);
                dup2(devNull, STDOUT_FILENO);
                close(devNull);
            }else{ // if in forground enable SIGINT
                ignact.sa_handler = SIG_DFL;
                sigaction(SIGINT, &ignact, NULL);
            }

            // Redirect input if '<' is present
            int i;
            for (i = 0; array[i] != NULL; i++) {
                if (strcmp(array[i], "<") == 0) {
                    int fd = open(array[i + 1], O_RDONLY); // open file you want to input to
                    if (fd == -1) {
                        perror("open");
                        exit(1);
                    }
                    dup2(fd, STDIN_FILENO); // change where stdin points
                    close(fd);
                    array[i] = NULL; // Remove the '<' from the argument list
                    break;
                }
            }

            // Redirect output if '>' is present
            for (i = 0; array[i] != NULL; i++) {
                if (strcmp(array[i], ">") == 0) {
                    int fd = open(array[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); //open file you want to output to
                    if (fd == -1) {
                        perror("open");
                        exit(1);
                    }
                    dup2(fd, STDOUT_FILENO); // change where stdout goes
                    close(fd);
                    array[i] = NULL; // Remove the '>' from the argument list
                    break;
                }
            }
            
            execvp(array[0], (char* const*)array); // execute command with arguments

            printf("Invalid Command\n"); // if commands fails
            fflush(stdout);
            exit(1);
            break;
        default:
            if (!runInBackground) { // if foreground process
                int status;
                pid_t result;
                do {
                    result = waitpid(childPid, &status, 0); // wait until child dies to do next command
                    if (result == -1) {
                        perror("waitpid");
                        exit(EXIT_FAILURE);
                    }
                    if (result == 0) {
                        // Child process still running, parent can do other things here
                        usleep(100000); // Sleep for 100 milliseconds to avoid busy waiting
                    }
                } while (result == 0);
                    if (WIFSIGNALED(status)) { // check to see if terminated by signal
                        printf("\nterminated by signal %d.\n", WTERMSIG(status)); // what kind of signal.
                        fflush(stdout);
                    }
                recentstatus = status;
            } else {
                int status;
                printf("Background process started with PID: %d\n", childPid);
                fflush(stdout);
                waitpid(childPid, &status, WNOHANG); // dont wait for child to die if in background process
                // Store the background process information
                bg_processes[num_bg_processes].pid = childPid;
                bg_processes[num_bg_processes].exit_status = 0; // Initialize to 0
                bg_processes[num_bg_processes].signaled = 0; // Initialize to 0
                num_bg_processes++;
            }
            break;
    }
}


void lastStatus(int status){
    if (WIFEXITED(status)) { // if exited normally

        printf("exited with value %d.\n", WEXITSTATUS(status));
        fflush(stdout);
        
    } else if (WIFSIGNALED(status)) { // if terminated by signal

        printf("\nterminated by signal %d.\n", WTERMSIG(status));
        fflush(stdout);

    }
}

void cd(char* array[], int argc) {
    if (argc == 1) {

        char *home_dir = getenv("HOME"); // gets home directory.
        chdir(home_dir); // changes dir to home directory

    } else if (argc > 2) { // if more than 2 argc in cd call then too many

        printf("Too many arguments for cd call\n");
        fflush(stdout);

    } else {

        if (chdir(array[1]) == -1) {
            perror("chdir");
        }else{
            chdir(array[1]); // change to directory user provided if its valid
        }
    }
}

void cmdlinehandler(char* array[], int argc, struct sigaction ignact) {
    if (strcmp(array[0], "cd") == 0) { // if cd command called
        cd(array, argc); // run cd function
    }else if(strcmp(array[0], "status") == 0){ // if status commmand called
        lastStatus(recentstatus); // run status command
    }else if(strstr(array[0], "#")){ // if # is in the start of cmd line input ignore it as it is a comment
        return;
    }else{
        execute(array, ignact, argc); // execute command user provided.
    }
}

char* $$conversion(char input[]) {

    char modified_input[MAXLENGTH]; // For storing the modified input string
    strcpy(modified_input, input);
    char *found = strstr(modified_input, "$$");

    int pid_length = snprintf(NULL, 0, "%d", originalPID);
    char pid_str[pid_length + 1]; // Add one for null terminator
    sprintf(pid_str, "%d", originalPID);

    while (found != NULL) {
        int found_length = 2; // Length of $$
        // Replace $$ with originalPID
        memmove(found + pid_length, found + found_length, strlen(found + found_length) + 1);
        memcpy(found, pid_str, pid_length);
        found = strstr(found + pid_length, "$$"); // Search for next occurrence of $$
    }

    return strdup(modified_input); // Return a dynamically allocated copy
}

void sigtstpFunc(){
	if (fgbool == 0){ // if foregroundonly mode is off
		fgbool = 1;	
		const char* string = "\nEntering foreground-only mode (& is now ignored)\n: "; 
		write(STDOUT_FILENO, string, 52); // write to stdout
		fflush(stdout);
		
	}else{ // if foregroundonly mode is on
		fgbool = 0;
        const char* string = "\nExiting foreground-only mode\n: ";
		write(STDOUT_FILENO, string, 32); // write to stdout
		fflush(stdout);
		
	}
}

int main() {
    struct sigaction ignact = {0};
    ignact.sa_handler = SIG_IGN;
    sigaction(SIGINT, &ignact, NULL); // turns of SIGINT so you cant ctrl c parent process

    struct sigaction sigStpAct={0};	
	sigStpAct.sa_handler = &sigtstpFunc;
	sigfillset(&sigStpAct.sa_mask);
	sigStpAct.sa_flags=SA_RESTART;
	sigaction(SIGTSTP, &sigStpAct, NULL); // turns off SIGSTP so you cant ctrl Z parent process

    char input[MAXLENGTH]; // Increased size to accommodate larger inputs
    char *modified_input; // For storing the modified input string
    char *array[MAXARGS]; // Changed to an array of pointers to char
    int argc = 0; // Variable to keep track of the number of arguments
    originalPID = getpid();

    printf("$ smallsh\n");
    fflush(stdout);
    while (1) { // Infinite loop until "exit" is entered
        printf(": ");
        fflush(stdout);

        fgets(input, sizeof(input), stdin); // Read a line of input including spaces
        input[strcspn(input, "\n")] = '\0'; // Remove the newline character

        modified_input = $$conversion(input); // Convert $$ in the input

        char *token = strtok(modified_input, " ");
        argc = 0; // Initialize argc for each input

        while (token != NULL && argc < MAXARGS) {
            array[argc] = (char*)malloc(strlen(token) + 1); // Allocate memory for each stringg
            strcpy(array[argc], token);
            token = strtok(NULL, " ");
            argc++;
        }

        if (argc > 0 && strcmp(array[0], "exit") == 0) // Check if the first argument is "exit"
            break;
        
        if (argc > 0) { // Check if there are any command-line arguments
            array[argc] = NULL;
            cmdlinehandler(array, argc, ignact);
            int i;
            for (i= 0; i < argc; i++) {
                free(array[i]); // Free dynamically allocated memory
            }
        }

        handle_bg_process_completion();

        free(modified_input); // Free dynamically allocated memory for modified_input
    }
    printf("$");


    return 0;
}
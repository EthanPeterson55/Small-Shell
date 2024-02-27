#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define MAXLENGTH 2048
#define MAXARGS 512
#define MAX_BG_PROCESSES 100

typedef struct {
    pid_t pid;
    int exit_status;
    int signaled;
} BackgroundProcess;

pid_t originalPID;
int fgbool = 0;
int recentstatus;

BackgroundProcess bg_processes[MAX_BG_PROCESSES];
int num_bg_processes = 0;

void handle_bg_process_completion();
void execute(char* array[], struct sigaction ignact, int argc);
void lastStatus(int status);
void cd(char* array[], int argc);
void cmdlinehandler(char* array[], int argc, struct sigaction ignact);
char* $$conversion(char input[]);
void sigtstpFunc();
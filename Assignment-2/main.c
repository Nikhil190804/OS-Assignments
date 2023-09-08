//list all the libs used
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>


//define the strcut for the command record
struct Record_of_Command {
    char command[1000];
    pid_t pid;
    time_t startTime;
    double duration;
    double memoryUsage;
};


void ctrl_c_pressed(){
    // call the necessary func here for cleanup and termination 
    printf("\n now ending it ");
    exit(EXIT_SUCCESS);
}


void add_record_to_history(struct Record_of_Command History[],int *number_of_commands,const char *command, pid_t pid, time_t startTime, double duration,double memoryUsage){
        struct Record_of_Command *record = &History[*number_of_commands];
        strncpy(record->command, command, sizeof(record->command));
        record->pid = pid;
        record->startTime = startTime;
        record->duration = duration;
        record->memoryUsage = memoryUsage;
        (*number_of_commands)++;
}


void display_records(struct Record_of_Command History[],int number_of_commands){
    printf("Commands executed are as follows..........");
    int i=0;
    while(i<number_of_commands){
        printf("--------------------");
        printf("Command: %s\n", History[i].command);
        printf("PID: %d\n", History[i].pid);
        printf("Start Time: %s", ctime(&History[i].startTime)); 
        printf("Duration: %.2f seconds\n", History[i].duration);
        printf("Memory Usage: %.2f KB\n", History[i].memoryUsage);
        printf("--------------------");
        printf("\n");
    }
}


//main func goes here
int main(){
    int number_of_commands=0;
    signal(SIGINT, ctrl_c_pressed);     //process the ctrl+c here 
    bool start_loop=true;
    char input[1000];
    struct Record_of_Command History[5000];
    printf("Welcome to Custom C Shell.....");
    while(start_loop){
        printf("\n"); 
        printf(">> : ");
        fgets(input,sizeof(input),stdin);
        size_t input_length = strlen(input);
        if (input_length > 0 && input[input_length - 1] == '\n') {
            input[input_length - 1] = '\0';
        }
        // Process the user input
        printf("You entered: %s\n", input);
    }
    return 0;
}

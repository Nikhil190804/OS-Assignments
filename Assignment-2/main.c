// list all the libs used
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
#include <time.h>

// define the strcut for the command record
struct Record_of_Command
{
    char command[2000];
    pid_t pid;
    time_t startTime;
    double duration;
    double memoryUsage;
};

int number_of_commands = 0;
struct Record_of_Command History[6969];


//forward declaration of func
void ctrl_c_pressed();
void add_record_to_history(struct Record_of_Command History[], int *number_of_commands, const char *command, pid_t pid, time_t startTime, double duration, double memoryUsage);
void display_records(struct Record_of_Command History[], int number_of_commands);

void ctrl_c_pressed()
{
    // call the necessary func here for cleanup and termination
    printf("\nNow ending it ....\nWith history of each Command..........\n");
    display_records(History,number_of_commands);
    exit(EXIT_SUCCESS);
}

void add_record_to_history(struct Record_of_Command History[], int *number_of_commands, const char *command, pid_t pid, time_t startTime, double duration, double memoryUsage)
{
    struct Record_of_Command *record = &History[*number_of_commands];
    strncpy(record->command, command, sizeof(record->command));
    record->pid = pid;
    record->startTime = startTime;
    record->duration = duration;
    record->memoryUsage = memoryUsage;
    (*number_of_commands)++;
}

void display_records(struct Record_of_Command History[], int number_of_commands)
{
    int i = 0;
    while (i < number_of_commands)
    {
        printf("--------------------\n");
        printf("Command: %s\n", History[i].command);
        printf("PID: %d\n", History[i].pid);
        printf("Start Time: %s", (char*)ctime(&History[i].startTime));
        printf("Duration: %.5f seconds\n", History[i].duration);
        printf("Memory Usage: %.2f KB\n", History[i].memoryUsage);
        printf("--------------------");
        printf("\n");
        i++;
    }
}

// main func goes here
int main()
{
    signal(SIGINT, ctrl_c_pressed); // process the ctrl+c here
    bool start_loop = true;
    char input[2000];
    printf("Welcome to Custom C Shell.....");
    while (start_loop)
    {
        printf("\n");
        printf(">> : ");
        fgets(input, sizeof(input), stdin);
        size_t input_length = strlen(input);
        if (input_length > 0 && input[input_length - 1] == '\n')
        {
            input[input_length - 1] = '\0';
        }
        // Process the user input
        int rc = fork();
        char *command[2000];

        if (rc < 0)
        {
            perror("Fork failed!!!");
            printf("\n");
            continue;
        }
        else if (rc == 0)
        {
            int count = 0;
            // Tokenize the input and store words in wordArray
            char *token = strtok(input, " "); // Split by spaces and newline characters
            while (token != NULL)
            {
                command[count++] = token;
                token = strtok(NULL, " \n");
            }
            if (execvp(command[0], command) == -1)
            {
                perror("Error in execution");
            }
        }
        else if (rc > 0)
        {
            int val;
            struct timeval startTime, endTime;
            gettimeofday(&startTime, NULL);
            waitpid(rc, &val, 0);
            if (WIFEXITED(val))
            {
                gettimeofday(&endTime, NULL);
                double dur = (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0;
                double memory;
                struct rusage usage;
                if (getrusage(RUSAGE_CHILDREN, &usage) == 0)
                {
                    memory = (double)usage.ru_maxrss / 1024.0;
                    add_record_to_history(History, &number_of_commands, input, rc, startTime.tv_sec, dur, memory);
                }
                else
                {
                    perror("getrusage");
                }
            }
            else
            {
                perror("child process did not exit normally");
            }
        }
    }
    return 0;
}

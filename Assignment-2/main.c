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
    int num_command;
};

//declare global vars here
int number_of_commands = 0;
struct Record_of_Command History[6969];
int record_pid_pipe_command[6969];
int view_index=0;
int insert_index=0;

// forward declaration of func
void ctrl_c_pressed();
void add_record_to_history(struct Record_of_Command History[], int *number_of_commands, const char *command, pid_t pid, time_t startTime, double duration, double memoryUsage,int num);
void display_records(struct Record_of_Command History[], int number_of_commands);
bool search_for_pipe_commands(char *input, size_t len);
char **parse_the_command(char *commands[], int i);
int *handle_me(char *input, size_t len);
double calculate_duration_of_time_intervals(struct timeval startTime,struct timeval endTime);
double calculate_memory_occupied_during_execution(struct rusage usage);
bool check_for_validity(char *checker,int input_length);

bool search_for_pipe_commands(char *input, size_t len)
{
    int index = 0;
    while (index < len)
    {
        if (input[index]=='|')
        {
            return true;
        }
        index++;
    }
    return false;
}

void ctrl_c_pressed()
{
    // call the necessary func here for cleanup and termination
    printf("\nNow ending it ....\nWith history of each Command..........\n");
    display_records(History, number_of_commands);
    exit(EXIT_SUCCESS);

}

bool check_for_validity(char *checker,int input_length){
    int i=0;
    while(i < input_length){
        if(checker[i]=='c' && i==0){
            if(checker[i+1]=='d'){
                return true;
            }
        }
        i++;
    }
    return false;
}

void add_record_to_history(struct Record_of_Command History[], int *number_of_commands, const char *command, pid_t pid, time_t startTime, double duration, double memoryUsage,int num)
{
    struct Record_of_Command *record = &History[*number_of_commands];
    strncpy(record->command, command, sizeof(record->command));
    record->pid = pid;
    record->startTime = startTime;
    record->duration = duration;
    record->memoryUsage = memoryUsage;
    record->num_command=num;
    (*number_of_commands)++;
}

void display_records(struct Record_of_Command History[], int number_of_commands)
{
    int i = 0;
    while (i < number_of_commands)
    {
        printf("--------------------\n");
        printf("Command: %s\n", History[i].command);
        if(History[i].pid < 0){
            // pipe command has taken over
            int loop=History[i].num_command;
            printf("PID: ");
            for(int j=0;j<loop;j++){
                printf("%d  ",record_pid_pipe_command[view_index]);
                view_index++;
            }
            printf("\n");
        }
        else{
            //normal pid print
            printf("PID: %d\n", History[i].pid);
        }
        printf("Start Time: %s", (char *)ctime(&History[i].startTime));
        printf("Duration: %.5f seconds\n", History[i].duration);
        printf("Memory Usage: %.2f KB\n", History[i].memoryUsage);
        printf("--------------------\n");
        i++;
    }
}

char **parse_the_command(char *commands[], int i)
{
    char **result= malloc(2000 * sizeof(char *));
    char *arg = strtok(commands[i], " ");
    int counter = 0;
    while (arg != NULL)
    {
        result[counter++] = arg;
        arg = strtok(NULL, " ");
    }
    result[counter] = NULL;
    return result;
}


int *handle_me(char *input, size_t len)
{   int *pid_array=calloc(100,sizeof(int));
    char *command[2000];
    int num_of_commands = 0;
    char *token = strtok(input, "|");
    while (token != NULL)
    {
        command[num_of_commands++] = token;
        token = strtok(NULL, "|");
    }
    int pipes[num_of_commands - 1][2];
    //int k=0;
    // while(k < num_of_commands-1){
    //     if(pipe(pipes[k])==-1){
    //         printf("\n Pipe Creation Failed!!!");
    //         exit(1);
    //     }
    //     k++;
    // }
    int input_file = 0;
    for (int i = 0; i < num_of_commands; i++)
    {
        int output_file = 1;
        int pipe_connections=pipe(pipes[i]);
        if(pipe_connections==-1){
            printf("\nPipe Creation Failed!!!\n");
            exit(1);
        }
        if (i < num_of_commands - 1)
        {
            output_file = pipes[i][1];
        }
        int rc = fork();
        if (rc < 0)
        {
            perror("could not create a process using fork");
            continue;
        }
        else if (rc == 0)
        {
            if (i > 0)
            {
                // If the current command is not the first one, redirect its input to read from the previous command's output, which is connected to the read end of the previous pipe.
                dup2(input_file, 0);
                close(input_file); // Close the previous input file descriptor.
            }
            if (i < num_of_commands - 1)
            {
                dup2(output_file, 1);
                close(output_file); // Close the previous output file descriptor
            }
            char **result = parse_the_command(command, i);
            execvp(result[0], result);
            printf("Not executed!!!\n");
        }
        else
        {
            wait(NULL);
            if (i < num_of_commands - 1)
            {
                close(pipes[i][1]);       // Close write end of the pipe
                input_file = pipes[i][0]; // Set input for the next command to read end of the pipe
            }
            pid_array[i]=rc;
        }
    }
    int x=0;
    int y=0;
    while(x < num_of_commands-1){
        close(pipes[x][0]);
        x++;
    }
    while(y < num_of_commands-1){
        close(pipes[y][1]);
        y++;
    }
    return pid_array;
}



double calculate_duration_of_time_intervals(struct timeval startTime,struct timeval endTime){
    double duration = (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0;
    return duration;
}


double calculate_memory_occupied_during_execution(struct rusage usage){
    getrusage(RUSAGE_CHILDREN, &usage);
    double mem;
    mem = (double)usage.ru_maxrss / 1024.0;
    return mem;
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
        char in[2000];
        strcpy(in,input);
        char checker[2000];
        strcpy(checker,input);
        bool status = check_for_validity(checker,input_length);
        if(status==true){
            printf("Can't execute this command!!!!!\n");
            continue;
        }
        bool result = search_for_pipe_commands(input, input_length);
        if (result == true)
        {   
            struct timeval startTime, endTime;
            gettimeofday(&startTime, NULL);
            int *pid_array=handle_me(input, input_length);
            gettimeofday(&endTime, NULL);
            int num=0;
            for(int i=0;pid_array[i]!=0;i++){
                record_pid_pipe_command[insert_index]=pid_array[i];
                insert_index++;
                num++;
            }
            double duration=calculate_duration_of_time_intervals(startTime,endTime);
            struct rusage usage;
            double mem = calculate_memory_occupied_during_execution(usage);
            add_record_to_history(History,&number_of_commands,in,-1,startTime.tv_sec,duration,mem,num);
            free(pid_array);
            continue;
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
            struct timeval start, end;
            gettimeofday(&start, NULL);
            waitpid(rc, &val, 0);
            if (WIFEXITED(val))
            {
                gettimeofday(&end, NULL);
                double dur = calculate_duration_of_time_intervals(start,end);
                struct rusage usage;
                double memory = calculate_memory_occupied_during_execution(usage);
                add_record_to_history(History, &number_of_commands, input, rc, start.tv_sec, dur, memory,0);
            }
            else
            {
                perror("child process did not exit normally");
            }
        }
    }
    return 0;
}

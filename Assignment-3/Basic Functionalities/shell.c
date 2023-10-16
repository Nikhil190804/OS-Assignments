#include <sys/resource.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>

#define MAX_STRINGS 1000
#define MAX_STRING_LENGTH 100
#define SHARED_MEM_SIZE (MAX_STRINGS * MAX_STRING_LENGTH)

pid_t scheduler_pid;
volatile sig_atomic_t alarm_triggered = 0;
bool start_loop = false;

struct my_history{
    char my_name[MAX_STRING_LENGTH];
    pid_t my_pid;
    long int execution_time;
    struct timeval start_time;
    struct timeval end_time;
    int flag;
};

struct SharedHistory{
    struct my_history array[MAX_STRINGS];
    int index_pointer;
    
};

struct SharedMemory {
    char strings[MAX_STRINGS][MAX_STRING_LENGTH];
    int index;
    int terminating_flag;
};

void terminator();

struct SharedMemory *create_shared_memory() {
    int shm_fd;
    struct SharedMemory *shared_mem;
    shm_fd = shm_open("/my_shared_memory", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    if (ftruncate(shm_fd, sizeof(struct SharedMemory))==-1) {
        perror("ftruncate");
        exit(1);
    }
    shared_mem = (struct SharedMemory *)mmap(0, sizeof(struct SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    shared_mem->terminating_flag=0;
    return shared_mem;
}


struct SharedHistory *create_shared_history() {
    int shm_fd;
    struct SharedHistory *shared_history;
    shm_fd = shm_open("/my_shared_history", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    if (ftruncate(shm_fd, sizeof(struct SharedHistory)) == -1) {
        perror("ftruncate");
        exit(1);
    }
    shared_history = (struct SharedHistory *)mmap(0, sizeof(struct SharedHistory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_history == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    shared_history->index_pointer = 0;
    return shared_history;
}

void shell_signal_handler(int signum)
{
    if (signum == SIGALRM)
    {   
        alarm_triggered = 1;
        
        printf("\ni am called:%d\n", scheduler_pid);
        
        kill(scheduler_pid, SIGCONT);
        pause();
    }
    else if(signum==SIGUSR1){
        kill(scheduler_pid,SIGSTOP);
        alarm(20);
    }
    else if(signum==SIGINT){
        terminator();
        //kill(scheduler_pid,SIGINT);
        //pause();
        //exit(EXIT_SUCCESS);
    }
}
struct SharedHistory *shared_history;
struct SharedMemory *shared_mem;

long int calculate_wait_time(struct timeval start_time, struct timeval end_time) {
    long int start_time_ms = start_time.tv_sec * 1000 + start_time.tv_usec / 1000;
    long int end_time_ms = end_time.tv_sec * 1000 + end_time.tv_usec / 1000;
    return end_time_ms - start_time_ms ;
}

void i_will_print_history(){
    int number_of_inputs=shared_history->index_pointer;
    printf("%d\n",number_of_inputs);
    printf("---------------History---------------\n");
    for(int i=0;i<number_of_inputs;i++){
        //pid_t t=shared_history->array[i]->my_pid;
        //printf("%d\n",t);
        if(shared_history->array[i].flag==-1){
            //valid hi nhi h
            continue;
        }
        printf("--------------------\n");
        printf("Name of Job :%s\n",shared_history->array[i].my_name);
        printf("PID: %d\n",shared_history->array[i].my_pid);
        printf("Execution Time: %ld ms\n",shared_history->array[i].execution_time);
        long int wait_time = calculate_wait_time(shared_history->array[i].start_time,shared_history->array[i].end_time) - shared_history->array->execution_time;
        printf("Wait Time : %ld ms\n",wait_time);
        printf("--------------------\n");
    }
}

void terminator(){
    start_loop=false;
    shared_mem->terminating_flag=1;
    i_will_print_history();
    kill(scheduler_pid,SIGCONT);
    usleep(10);
    kill(scheduler_pid,SIGTERM);
    //sleep(5);
    munmap(shared_mem, sizeof(struct SharedMemory));
    shm_unlink("/my_shared_memory");
    munmap(shared_history,sizeof(struct SharedHistory));
    shm_unlink("/my_shared_history");
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Provide all arguments!!!!!\n");
        exit(1);
    }
    struct sigaction signal;
    signal.sa_handler = shell_signal_handler; 
    sigaction(SIGALRM, &signal, NULL);
    sigaction(SIGUSR1,&signal,NULL);
    sigaction(SIGINT,&signal,NULL);
    int num_of_cpu = atoi(argv[1]);
    int time_slice = atoi(argv[2]);
    scheduler_pid = fork();
    if (scheduler_pid == 0)
    {
        execlp("./temps", "./temps", argv[1], argv[2], NULL);
        printf("Failed to start!!!!!\n");
        exit(1);
    }
    else if (scheduler_pid < 0)
    {
        printf("\nFailed to start!!!!!\n");
        exit(1);
    }
    sleep(2);
    kill(scheduler_pid, SIGSTOP);
    start_loop = true;
    char input[2000];
    char *command[2000];
    alarm(20);
    shared_mem = create_shared_memory();
    shared_history = create_shared_history();
    shared_mem->index=0;
    while (start_loop)
    {
        if (alarm_triggered == 1)
        {   
            for(int i=0;i<shared_mem->index;i++){
                strcpy(shared_mem->strings[i],"");
            }
            shared_mem->index=0;
            alarm_triggered = 0;
            while (getchar() != '\n');
            continue;
        }
        printf("\n");
        printf(">> : ");
        if (fgets(input, sizeof(input), stdin) != NULL)
        {
            size_t input_length = strlen(input);
            if (input_length > 0 && input[input_length - 1] == '\n')
            {
                input[input_length - 1] = '\0';
            }
            /*char *command[2000];
            char *token = strtok(input, " ");
            // Split by spaces and newline characters
            int count = 0;
            while (token != NULL)
            {
                command[count++] = token;
                token = strtok(NULL, " \n");
            }*/
            int ind=shared_mem->index;
            strcpy(shared_mem->strings[ind],input);
            
            shared_mem->index++;
            printf("%s...\n",shared_mem->strings[ind]);
            if(strcmp(input,"exit")==0){
                /*munmap(shared_mem, sizeof(struct SharedMemory));
                shm_unlink("/my_shared_memory");
                start_loop=false;
                i_will_print_history();
                kill(scheduler_pid,SIGCONT);
                //usleep(100);
                kill(scheduler_pid,SIGTERM);
                //sleep(1);
                munmap(shared_history,sizeof(struct SharedHistory));
                shm_unlink("/my_shared_history");
                exit(0);*/
                terminator();
            }
        }
    }
    munmap(shared_mem, SHARED_MEM_SIZE);
    shm_unlink("/my_shared_memory");
    printf("parent here %d", getpid());
}
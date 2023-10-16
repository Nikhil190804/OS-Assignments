/*#include <sys/resource.h>
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

pid_t scheduler_pid;
volatile sig_atomic_t alarm_triggered = 0;

void alarm_signal_handler(int signum)
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
        alarm(15);
    }
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Provide all arguments!!!!!\n");
        exit(1);
    }
    struct sigaction signal;
    signal.sa_handler = alarm_signal_handler; 
    sigaction(SIGALRM, &signal, NULL);
    sigaction(SIGUSR1,&signal,NULL);
    int num_of_cpu = atoi(argv[1]);
    int time_slice = atoi(argv[2]);
    scheduler_pid = fork();

    if (scheduler_pid == 0)
    {
        execlp("./scheduler", "./scheduler", argv[1], argv[2], NULL);
        printf("Failed to start!!!!!\n");
        exit(1);
    }
    else if (scheduler_pid < 0)
    {
        printf("\nFailed to start!!!!!\n");
        exit(1);
    }
    usleep(100000);
    kill(scheduler_pid, SIGSTOP);
    bool start_loop = true;
    char input[2000];
    char *command[2000];
    alarm(15);
    while (start_loop)
    {
        if (alarm_triggered == 1)
        {
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
            char *command[2000];
            char *token = strtok(input, " ");
            // Split by spaces and newline characters
            int count = 0;
            while (token != NULL)
            {
                command[count++] = token;
                token = strtok(NULL, " \n");
            }
            printf("%s\n", input);
        }
    }
    printf("parent here %d", getpid());
}*/


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

#define MAX_STRINGS 100
#define MAX_STRING_LENGTH 100

volatile sig_atomic_t start_now = 0;
volatile sig_atomic_t var = 0;
int NCPU=0;
int TSLICE=0;

struct SharedMemory
{
    char strings[MAX_STRINGS][MAX_STRING_LENGTH];
    int index;
};

struct SharedMemory *access_shared_memory()
{
    int shm_fd;
    struct SharedMemory *shared_mem;
    shm_fd = shm_open("/my_shared_memory", O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(1);
    }
    shared_mem = (struct SharedMemory *)mmap(0, sizeof(struct SharedMemory), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
    return shared_mem;
}

struct SharedMemory *shared_mem;

void memory_clear(struct SharedMemory *shared_mem)
{
    munmap(shared_mem, sizeof(struct SharedMemory));
    return;
}

void signal_handler(int signum)
{
    if (signum == SIGCONT)
    {
        printf("\nscheduler here...\n");
        start_now = 1;
        var = 0;
    }
    else if (signum == SIGTERM)
    {
        printf("now terminating scheduler..\n");
        memory_clear(shared_mem);
        exit(0);
    }
}

struct SharedMemory *shared_mem;
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Provide all arguments!!!!!\n");
        exit(1);
    }
    NCPU=atoi(argv[1]);
    TSLICE=atoi(argv[2]);
    struct sigaction signal;
    signal.sa_handler = signal_handler;
    sigaction(SIGCONT, &signal, NULL);
    sigaction(SIGTERM, &signal, NULL);
    pid_t my_parent = getppid();
    printf("%d...\n", my_parent);
    shared_mem = access_shared_memory();
    while (true)
    {
        if (start_now == 1)
        {
            int number_of_strings = shared_mem->index;
            number_of_strings++;
            while (true)
            {
                printf("%d\n", var);
                printf("%s\n", shared_mem->strings[var]);
                var++;
                if (var >= number_of_strings)
                {
                    start_now = 0;
                    kill(my_parent, SIGUSR1);
                    break;
                }
            }
        }
        else
        {
            ;
        }
    }
}
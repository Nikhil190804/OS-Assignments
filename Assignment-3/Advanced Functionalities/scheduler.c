
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

volatile sig_atomic_t start_now = 0;
volatile sig_atomic_t var = 0;
int NCPU = 0;
int TSLICE = 0;

struct SharedMemory
{
    char strings[MAX_STRINGS][MAX_STRING_LENGTH];
    int index;
};

typedef struct
{
    pid_t data[MAX_STRINGS];
    int front;
    int rear;
} StringQueue;

StringQueue *createStringQueue()
{
    StringQueue *queue = (StringQueue *)malloc(sizeof(StringQueue));
    queue->front = 0;
    queue->rear = -1;
    return queue;
}

int isStringQueueEmpty(StringQueue *queue)
{
    return queue->front > queue->rear;
}

int isStringQueueFull(StringQueue *queue)
{
    return (queue->rear - queue->front) >= MAX_STRINGS - 1;
}

void enqueueString(StringQueue *queue, pid_t item)
{
    if (isStringQueueFull(queue))
    {
        printf("String Queue is full. Cannot enqueue.\n");
        return;
    }

    queue->rear++;
    queue->data[queue->rear] = item;
}

pid_t dequeueString(StringQueue *queue)
{
    if (isStringQueueEmpty(queue))
    {
        printf("String Queue is empty. Cannot dequeue.\n");
        return 0;
    }

    pid_t item = queue->data[queue->front];
    queue->front++;
    return item;
}

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
    shared_mem = (struct SharedMemory *)mmap(0, sizeof(struct SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
    return shared_mem;
}

struct SharedMemory *shared_mem;
StringQueue *queue ;
StringQueue *remaining_jobs;

void memory_clear(struct SharedMemory *shared_mem)
{
    munmap(shared_mem, sizeof(struct SharedMemory));
    free(queue);
    free(remaining_jobs);
    free(shared_mem);
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

pid_t peekString(StringQueue *queue)
{
    if (isStringQueueEmpty(queue))
    {
        printf("String Queue is empty. Cannot peek.\n");
        return 0;
    }

    return queue->data[queue->front];
}


int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Provide all arguments!!!!!\n");
        exit(1);
    }
    NCPU = atoi(argv[1]);
    TSLICE = atoi(argv[2]);
    struct sigaction signal;
    signal.sa_handler = signal_handler;
    sigaction(SIGCONT, &signal, NULL);
    sigaction(SIGTERM, &signal, NULL);
    pid_t my_parent = getppid();
    printf("%d...\n", my_parent);
    shared_mem = access_shared_memory();
    printf("f\n");
    queue = createStringQueue();
    remaining_jobs = createStringQueue();
    printf("%d::%d\n", NCPU, TSLICE);
    while (true)
    {
        if (start_now == 1)
        {   
            int number_of_strings = shared_mem->index;
            printf("index val in shared: %d",shared_mem->index);
            while (true)
            {

                if (var >= number_of_strings)
                {
                    // start_now = 0;
                    // kill(my_parent, SIGUSR1);
                    break;
                }
                // add check for data valid in shared_mem
                char *item = shared_mem->strings[var];
                printf("i am var: %d\n", var);
                printf("%s\n", item);
                pid_t new_child = fork();
                if (new_child == 0)
                {   
                    execlp(item, item, NULL);
                    printf("Failed to fork a process!!!!!\n");
                    exit(1);
                }
                else if (new_child < 0)
                {
                    printf("Failed to fork a process!!!!!\n");
                }
                else
                {
                    kill(new_child, SIGSTOP);
                }
                enqueueString(queue, new_child);
                var++;
            }
            while (true)
            {
                if (isStringQueueEmpty(remaining_jobs))
                {
                    break;
                }
                else
                {
                    pid_t old_child = dequeueString(remaining_jobs);
                    enqueueString(queue, old_child);
                }
            }
            if (isStringQueueEmpty(queue))
            {
                start_now = 0;
                kill(my_parent, SIGUSR1);
                usleep(10000);
            }
            else
            {
                for (int i = 0; i < NCPU; i++)
                {   
                    if (isStringQueueEmpty(queue))
                    {
                        printf("No process to run now.....\n");
                        start_now = 0;
                        kill(my_parent, SIGUSR1);
                        usleep(10000);
                    }
                    pid_t child_pid = dequeueString(queue);
                    int status;
                    kill(child_pid, SIGCONT);
                    usleep(1000 * TSLICE);
                    int result = waitpid(child_pid, &status, WNOHANG);
                    if (result == 0)
                    {
                        printf("need time\n");
                        kill(child_pid, SIGSTOP);
                        enqueueString(remaining_jobs, child_pid);
                    }
                    else if (result == child_pid)
                    {
                        if (WIFEXITED(status))
                        {
                            printf("Child process exited with status: %d\n", WEXITSTATUS(status));
                        }
                        else
                        {
                            printf("Child process terminated abnormally\n");
                        }
                    }
                    else
                    {
                        ;
                    }
                }
                start_now = 0;
                kill(my_parent, SIGUSR1);
            }
        }
        else
        {
            ;
        }
    }
}
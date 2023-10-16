
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

struct my_history
{
    char my_name[MAX_STRING_LENGTH];
    pid_t my_pid;
    long int execution_time;
    struct timeval start_time;
    struct timeval end_time;
    int flag;
};

struct SharedMemory
{
    char strings[MAX_STRINGS][MAX_STRING_LENGTH];
    int index;
    int terminating_flag;
};

struct SharedHistory
{
    struct my_history array[MAX_STRINGS];
    int index_pointer;
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

int countStringQueue(StringQueue *queue)
{
    return (queue->rear - queue->front + 1);
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
        perror("my_shared_memory : shm_open");
        exit(1);
    }
    shared_mem = (struct SharedMemory *)mmap(0, sizeof(struct SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
    shared_mem->terminating_flag = 0;
    return shared_mem;
}

struct SharedHistory *create_shared_history()
{
    int shm_fd;
    struct SharedHistory *shared_history;
    shm_fd = shm_open("/my_shared_history", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("my_shared_history : shm_open");
        exit(1);
    }
    if (ftruncate(shm_fd, sizeof(struct SharedHistory)) == -1)
    {
        perror("ftruncate");
        exit(1);
    }
    shared_history = (struct SharedHistory *)mmap(0, sizeof(struct SharedHistory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_history == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }

    shared_history->index_pointer = 0;
    return shared_history;
}

struct SharedMemory *shared_mem;
StringQueue *queue;
StringQueue *remaining_jobs;
struct SharedHistory *shared_history;
pid_t my_parent;

void memory_clear(struct SharedMemory *shared_mem)
{
    munmap(shared_mem, sizeof(struct SharedMemory));
    free(queue);
    free(remaining_jobs);
    return;
}

void get_current_time(struct timeval *current_time)
{
    gettimeofday(current_time, NULL);
}

void check_for_more_jobs()
{
    if (isStringQueueEmpty(queue))
    {
        return;
    }
    else
    {
        int num = countStringQueue(queue);
        for (int i = 0; i < num; i++)
        {
            pid_t child_pid = dequeueString(queue);
            int status;
            kill(child_pid, SIGCONT);
            int result = waitpid(child_pid, &status,0);
            if (result == child_pid)
            {
                if (WIFEXITED(status))
                {   
                    printf("Child process with pid: %d exited with status: %d\n",child_pid, WEXITSTATUS(status));
                }
                else
                {
                    printf("Child process with pid: %d terminated abnormally\n",child_pid);
                }
            }
            else{
                ;
            }
        }
    }
}

void signal_handler(int signum)
{
    if (signum == SIGCONT)
    {
        if (shared_mem->terminating_flag == 0)
        {
            start_now = 1;
            var = 0;
        }
    }
    else if (signum == SIGTERM)
    {
        check_for_more_jobs();
        usleep(10);
        memory_clear(shared_mem);
        munmap(shared_history, sizeof(struct SharedHistory));
        shm_unlink("/my_shared_memory");
        shm_unlink("/my_shared_history");
        exit(0);
    }
}

pid_t peekString(StringQueue *queue)
{
    if (isStringQueueEmpty(queue))
    {
        return 0;
    }

    return queue->data[queue->front];
}

int find_valid_index_in_shared_history(pid_t item)
{
    int number_of_items = shared_history->index_pointer;
    for (int i = 0; i < number_of_items; i++)
    {
        if (shared_history->array[i].my_pid == item)
        {
            return i;
        }
    }
    return -1;
}

struct SharedMemory *shared_mem;
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Provide all arguments!!!!!\n");
        exit(1);
    }
    NCPU = atoi(argv[1]);
    TSLICE = atoi(argv[2]);
    struct sigaction signal_scheduler;
    signal_scheduler.sa_handler = signal_handler;
    sigaction(SIGCONT, &signal_scheduler, NULL);
    sigaction(SIGTERM, &signal_scheduler, NULL);
    my_parent = getppid();
    shared_mem = access_shared_memory();
    shared_history = create_shared_history();
    queue = createStringQueue();
    remaining_jobs = createStringQueue();
    setsid();
    while (true)
    {
        if (start_now == 1)
        {
            int number_of_strings = shared_mem->index;
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
                struct my_history entry;
                // entry = (struct my_history *)malloc(sizeof(struct my_history));
                pid_t new_child = fork();
                if (new_child == 0)
                {
                    signal(SIGCONT, SIG_DFL);
                    usleep(10000);
                    execlp(item, item, NULL);
                    perror("error here: %s");
                    exit(1);
                }
                else if (new_child > 0)
                {
                    usleep(10000);
                    kill(new_child, SIGSTOP);
                }
                else
                {
                    printf("Failed to fork a process!!!!!\n");
                }
                enqueueString(queue, new_child);
                strcpy(entry.my_name, item);
                entry.my_pid = new_child;
                entry.execution_time = 0;
                entry.flag = -1;
                get_current_time(&entry.start_time);
                int p = shared_history->index_pointer;
                shared_history->array[p] = entry;
                shared_history->index_pointer++;
                // free(entry);
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
                    if (old_child == 0)
                    {
                        continue;
                    }
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
                        // start_now = 0;
                        continue;
                        // kill(my_parent, SIGUSR1);
                    }
                    pid_t child_pid = dequeueString(queue);
                    if (child_pid == 0)
                    {
                        continue;
                    }
                    int status;
                    kill(child_pid, SIGCONT);
                    usleep(1000 * TSLICE);
                    int result = waitpid(child_pid, &status, WNOHANG);
                    if (result == 0)
                    {
                        kill(child_pid, SIGSTOP);
                        enqueueString(remaining_jobs, child_pid);
                        int updater = find_valid_index_in_shared_history(child_pid);
                        if (updater != -1)
                        {
                            shared_history->array[updater].execution_time += (long int)TSLICE;
                            get_current_time(&shared_history->array[updater].end_time);
                            shared_history->array[updater].flag = 0;
                        }
                    }
                    else if (result == child_pid)
                    {
                        if (WIFEXITED(status))
                        {
                            printf("Child process with pid: %d exited with status: %d\n",child_pid, WEXITSTATUS(status));
                            int updater = find_valid_index_in_shared_history(child_pid);
                            if (updater != -1)
                            {
                                shared_history->array[updater].execution_time += (long int)TSLICE;
                                get_current_time(&shared_history->array[updater].end_time);
                                shared_history->array[updater].flag = 1;
                            }
                            else
                            {
                                printf("Child with pid: %d Didn't ran successfully!!!!!\n",child_pid);
                            }
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
                if (isStringQueueEmpty(queue))
                {
                    // start_now = 0;
                    // kill(my_parent, SIGUSR1);
                    ;
                }
                else
                {
                    int num_of_current_elements = countStringQueue(queue);
                    if (num_of_current_elements == 0)
                    {
                        ;
                    }
                    else
                    {
                        for (int i = 0; i < num_of_current_elements; i++)
                        {
                            if (isStringQueueEmpty(queue))
                            {
                                break;
                            }
                            else
                            {
                                pid_t temp_child = dequeueString(queue);
                                kill(temp_child, SIGCONT);
                                usleep(10);
                                kill(temp_child, SIGSTOP);
                                enqueueString(queue, temp_child);
                            }
                        }
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
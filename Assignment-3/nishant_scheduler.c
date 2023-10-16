#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <stdbool.h>

#define MAX_INPUT_SIZE 1024
#define MAX_QUEUE_SIZE 1024
#define CUSTOM_START_SIGNAL SIGUSR2
#define CUSTOM_STOP_SIGNAL SIGUSR1

typedef struct {
    char command[MAX_INPUT_SIZE];
    pid_t pid;
    int priority;
    int executionTime;
    int waitTime;
} ProcessInfo;

ProcessInfo *queue;
int shmid;
int readyQueueCount = 0;
int NCPU = 1;
int TSLICE = 4000000;  // 4 seconds (in microseconds)

void startProcess(int signum) {
    // Execute the process
    execlp(queue[0].command, queue[0].command, (char *)NULL);
    perror("execlp"); // Handle execlp error
    _exit(EXIT_FAILURE);
}

void addToQueue(ProcessInfo *queue, const char *command, pid_t pid) {
    if (readyQueueCount < MAX_QUEUE_SIZE) {
        // Set a default priority (you can change this value as needed)
        int priority = 1;

        strcpy(queue[readyQueueCount].command, command);
        queue[readyQueueCount].pid = pid;
        queue[readyQueueCount].priority = priority;
        queue[readyQueueCount].executionTime = 0;
        queue[readyQueueCount].waitTime = 0;
        readyQueueCount++;
    }
}

int createQueue() {
    key_t key = ftok("/tmp", 'A'); // Generate a unique key
    shmid = shmget(key, MAX_QUEUE_SIZE * sizeof(ProcessInfo), IPC_CREAT | 0666);

    if (shmid < 0) {
        perror("shmget");
        return -1;
    }

    queue = (ProcessInfo *)shmat(shmid, NULL, 0);
    if (queue == (ProcessInfo *)(-1)) {
        perror("shmat");
        return -1;
    }

    return shmid;
}

void removeQueue(int shmid) {
    shmdt(queue);
    shmctl(shmid, IPC_RMID, NULL);
}

void executeNextProcess(int signum) {
    if (readyQueueCount > 0) {
        // Send a signal to start the next process
        kill(queue[0].pid, CUSTOM_START_SIGNAL);
    }
}

void stopRunningProcesses(int signum) {
    if (readyQueueCount > 0) {
        // Send a signal to stop the currently running process
        kill(queue[0].pid, CUSTOM_STOP_SIGNAL);
    }
}

int main(int argc, char *argv[]) {
    int status;
    if (argc != 3) {
        printf("Usage: %s NCPU TSLICE (in microseconds)\n", argv[0]);
        return EXIT_FAILURE;
    }

    NCPU = atoi(argv[1]);
    TSLICE = atoi(argv[2]);

    pid_t scheduler_pid = fork();
    if (scheduler_pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (scheduler_pid == 0) {
        // This is the scheduler process
        signal(CUSTOM_START_SIGNAL, startProcess);
        signal(SIGALRM, executeNextProcess);

        while (1) {
            // Scheduler logic
            if (readyQueueCount > 0) {
                // If there are processes in the ready queue, execute them in a round-robin manner
                for (int i = 0; i < readyQueueCount; i++) {
                    // Send a signal to start the process
                    kill(queue[0].pid, CUSTOM_START_SIGNAL);
                    // Wait for the process to finish
                    int status;
                    waitpid(queue[0].pid, &status, 0);
                    // Update the start time
                    // Remove the completed process from the queue
                    for (int j = 0; j < readyQueueCount - 1; j++) {
                        queue[j] = queue[j + 1];
                    }
                    readyQueueCount--;
                    // Send a signal to stop the process
                    kill(queue[0].pid, CUSTOM_STOP_SIGNAL);
                }
            }
        }
        exit(EXIT_SUCCESS);
    }

    // This is the parent process, the shell
    shmid = createQueue();
    signal(SIGINT, SIG_IGN); // Ignore Ctrl+C in the scheduler
    signal(CUSTOM_STOP_SIGNAL, stopRunningProcesses);

    while (1) {
        // Scheduler execution logic
    }

    // Cleanup and remove the shared queue before exiting
    removeQueue(shmid);
    exit(EXIT_SUCCESS);
}
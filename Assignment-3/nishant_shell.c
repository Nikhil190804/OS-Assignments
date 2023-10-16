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

void pressed_ctrlC() {
    printf("TERMINATING THE PROCESS\n");
    pid_t n = fork();
        if(n==0){
            execlp("./nishant_scheduler","./nishant_scheduler",NULL);
        }
        else{
            wait(NULL);
            printf("m to chla");
            exit(0);
        }
    
    // Cleanup and remove the shared queue before exiting
    
    //exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int status;
    if (argc != 3) {
        printf("Usage: %s NCPU TSLICE (in microseconds)\n", argv[0]);
        return EXIT_FAILURE;
    }

    NCPU = atoi(argv[1]);
    TSLICE = atoi(argv[2]);

    signal(SIGINT, pressed_ctrlC);
    shmid = createQueue();
    signal(SIGALRM, executeNextProcess);

    printf("HELLO!! WELCOME TO CUSTOMIZED C SHELL\n");

    struct timeval startTime, currentTime;
    gettimeofday(&startTime, NULL);
    
    int shellInterval = 5;  // Set the interval for the shell to run (e.g., 5 seconds)
    int shellRunning = 1;

    while (1) {
        // Check if it's time to run the shell
        gettimeofday(&currentTime, NULL);
        long elapsed = (currentTime.tv_sec - startTime.tv_sec);
        
        if (elapsed >= shellInterval) {
            shellRunning = 1;
            gettimeofday(&startTime, NULL);

            if (shellRunning) {
                printf("My_Shell--> ");
                char input[MAX_INPUT_SIZE];
                char *inputStr = fgets(input, sizeof(input), stdin);
                if (inputStr == NULL) {
                    perror("fgets");
                    exit(EXIT_FAILURE);
                } else {
                    inputStr[strcspn(inputStr, "\n")] = '\0';

                    if (strcmp(inputStr, "exit") == 0) {
                        // Cleanup and remove the shared queue before exiting
                        removeQueue(shmid);
                        printf("Exiting the shell...\n");
                        break;
                    } else {
                        pid_t newpid = fork();
                        if (newpid == 0) {
                            // Child process
                            // Execute the command in the child process
                            execlp(inputStr, inputStr, (char *)NULL);
                            perror("execlp"); // Handle execlp error
                            _exit(EXIT_FAILURE);
                        } else if (newpid < 0) {
                            // Handle fork error
                        } else {
                            // Parent process
                            kill(newpid,SIGSTOP);
                            addToQueue(queue, inputStr, newpid);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
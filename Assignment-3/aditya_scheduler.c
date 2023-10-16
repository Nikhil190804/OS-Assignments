#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>

// Define a key for the shared memory segment
#define SHM_KEY 12345
#define SHM_SIZE 1024

// Define a structure for a queue node
typedef struct QueueNode {
    char data[256];
    struct QueueNode *next;
} QueueNode;

// Define a structure for a queue
typedef struct {
    QueueNode *front;
    QueueNode *rear;
} Queue;

// Function to initialize a queue
void initializeQueue(Queue *queue) {
    queue->front = queue->rear = NULL;
}

// Function to enqueue a new element in the queue
void enqueue(Queue *queue, char *data) {
    QueueNode *newNode = (QueueNode *)malloc(sizeof(QueueNode));
    if (newNode == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(newNode->data, data, sizeof(newNode->data));
    newNode->next = NULL;

    if (queue->rear == NULL) {
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

// Function to dequeue an element from the queue
char *dequeue(Queue *queue) {
    if (queue->front == NULL) {
        return NULL; // Queue is empty
    }

    QueueNode *temp = queue->front;
    queue->front = temp->next;

    if (queue->front == NULL) {
        queue->rear = NULL; // The last element has been dequeued
    }

    char *data = strdup(temp->data);
    free(temp);
    return data;
}



// Define a key for the shared memory segment
#define SHM_KEY 12345
#define SHM_SIZE 1024

// Define the maximum number of processes (NCPU) to run in parallel
#define NCPU 4

// Define the time slice (TSLICE) in seconds
#define TSLICE 5

static volatile sig_atomic_t process_counter = 0;
static volatile sig_atomic_t scheduler_running = 1;

void sigchld_handler(int signo) {
    int status, child_pid;
    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            process_counter--;
        }
    }
}

void timer_handler(int signum) {
    scheduler_running = 0;
}
int main() {
    signal(SIGCHLD, sigchld_handler);
    signal(SIGALRM, timer_handler);

    // Create a shared memory segment
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Attach the shared memory segment
    char *shm_data = shmat(shmid, NULL, 0);
    if (shm_data == (void *)-1) {
        perror("shmat");
        exit(1);
    }

// Inside the scheduler code
// Inside the scheduler code
while (1) {
    scheduler_running = 1;
    process_counter = 0;

    while (scheduler_running) {
        if (strlen(shm_data) > 0) {
            char *input = strdup(shm_data);

            if (process_counter < NCPU) {
                process_counter++;
                pid_t pid = fork();

                if (pid == -1) {
                    perror("fork");
                } else if (pid == 0) {
                    // This is the child process
                    printf("Executing code from file: %s\n", input);

                    // Execute the C code in the specified file
                    char run_cmd[512];
                    snprintf(run_cmd, sizeof(run_cmd), "gcc -o temp_executable %s", input);
                    int compilation_result = system(run_cmd);

                    if (compilation_result == 0) {
                        // Compilation was successful
                        char execute_cmd[512];
                        snprintf(execute_cmd, sizeof(execute_cmd), "./temp_executable");

                        // Capture the output
                        FILE *output_file = popen(execute_cmd, "r");
                        if (output_file) {
                            char output_buffer[1024];
                            while (fgets(output_buffer, sizeof(output_buffer), output_file) != NULL) {
                                printf("%s", output_buffer); // Print output to the terminal
                            }
                            pclose(output_file);
                        }
                    } else {
                        printf("Compilation and execution failed for %s\n", input);
                    }

                    // Clean up
                    remove("temp_executable");
                    sleep(TSLICE);
                    exit(0);
                }
            }

            memset(shm_data, 0, SHM_SIZE);
            free(input);
        }

        // Sleep for a short interval before checking the shared memory again
        usleep(100000); // 100 milliseconds
    }

    // Wait for all child processes to complete
    while (process_counter > 0) {
        usleep(100000); // 100 milliseconds
    }
}
}
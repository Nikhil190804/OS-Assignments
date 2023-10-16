#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <setjmp.h>

// Define a key for the shared memory segment
#define SHM_KEY 12345
#define SHM_SIZE 1024

static jmp_buf env;
static int in_shell = 1;

void timer_handler(int signum) {
    // Switch between shell and scheduler
    if (in_shell) {
        in_shell = 0;
        longjmp(env, 1); // Jump to scheduler
    } else {
        in_shell = 1;
        longjmp(env, 1); // Jump back to shell
    }
}

int main() {
    signal(SIGALRM, timer_handler);

    // Create a shared memory segment
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    while (1) {
        if (setjmp(env) == 0) {
            if (in_shell) {
                // Shell code
                char input[256];

                // Prompt the user for input
                printf("Enter the name of an executable file (or 'exit' to quit): ");
                fgets(input, sizeof(input), stdin);

                // Remove the newline character from the input
                input[strcspn(input, "\n")] = 0;

                // Check if the user wants to exit
                if (strcmp(input, "exit") == 0) {
                    break;
                }

                // Attach the shared memory segment
                char *shm_data = shmat(shmid, NULL, 0);
                if (shm_data == (void *)-1) {
                    perror("shmat");
                    exit(1);
                }

                // Copy the input to the shared memory
                strncpy(shm_data, input, SHM_SIZE);

                // Detach the shared memory segment
                shmdt(shm_data);
            } else {
                // Scheduler code
                char *args[] = {"./aditya_scheduler", NULL};
                execvp("./aditya_scheduler", args);
                perror("execvp");
                exit(1);
            }
        }

        // Set the timer for 15 seconds
        alarm(15);
    }

    // Remove the shared memory segment when done
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
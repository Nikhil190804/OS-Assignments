#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define SHM_SIZE 4096 // Size of the shared memory region
#define MAX_STRINGS 10

// Define a key for the shared memory segment

// Define the maximum number of processes (NCPU) to run in parallel
#define NCPU 4

// Define the time slice (TSLICE) in seconds
#define TSLICE 5
pid_t mera_baap;

struct SharedData *shm_ptr;
struct SharedData
{
    char strings[MAX_STRINGS][256];
    int index;
};

void run()
{
    while (1)
    {
        printf("%d.....",shm_ptr->index);
        for (int i = 0; i < shm_ptr->index; i++)
        {
            pid_t bacha = fork();
            if (bacha == 0)
            {
                execlp(shm_ptr->strings[i], shm_ptr->strings[i], NULL);
                printf("dfg\n");
            }
            else
            {
                int st;
                int res = waitpid(bacha, &st, 0);
                if (WIFEXITED(st))
                {
                    printf("run succes\n");
                }
            }
        }
        // sleep(10);
        kill(mera_baap, SIGUSR1);
    }
}

// Function to i

void mera_handler(int s)
{
    if (s == SIGCONT)
    {   printf("dfghjkjhgfds\n");
        run();
    }
}

int main()
{
    int shm_fd;                 // File descriptor for the shared memory object
    // Pointer to the shared memory region
    signal(SIGCONT, mera_handler);
    // Create or open a shared memory object
    shm_fd = shm_open("/my_shared_memory", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(1);
    }
    shm_ptr = mmap(0, sizeof(struct SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
    mera_baap = getppid();
    printf("fssdfgbnfee\n");

    while (1)
    {
        ;
    }
}
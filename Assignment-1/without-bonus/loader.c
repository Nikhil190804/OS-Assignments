#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */

// memory cleanup function

void loader_cleanup()
{
    if (sizeof(ehdr) >= 0)
    {
        free(ehdr);
    }
    if (sizeof(phdr) >= 0)
    {
        free(phdr);
    }
    close(fd);
}

/*
 * Load and run the ELF executable file
 */

// Loading and Running the elf file

void load_and_run_elf(char **exe)
{
    fd = open(exe[1], O_RDONLY);

    if (fd < 0)
    {

        printf("Error in File Descriptor!!!");

        exit(1);
    }

    // int a=sizeof(ehdr);

    // printf("%d",a);

    // allocating the memory

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    ssize_t reader = read(fd, ehdr, sizeof(Elf32_Ehdr));

    // error handling cases

    if (reader <= 0)
    {
        printf("Error while reading the file!!!");
        exit(1);
    }
    Elf32_Phdr *phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));
    // ssize_t rea=read(fd, phdr, sizeof(Elf32_Phdr));

    // printf("ELF Type: %d\n", phdr.);
    // for(int i=)
    // printf("%d",ehdr->e_phnum);
    // Elf32_Phdr loadSegment;
    // printf("%d\n",ehdr->e_entry);
    // printf("Entry Point: 0x%lx\n", (unsigned long)ehdr->e_entry);

    if (ehdr->e_phnum == 0)
    {

        printf("Error in e_phnum!!!");

        exit(1);
    }

    // iterating the program header

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        // Elf32_Phdr *phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));
        // ssize_t rea=read(fd, phdr, sizeof(Elf32_Phdr));
        off_t ls = lseek(fd, ehdr->e_phoff + i * sizeof(Elf32_Phdr), SEEK_SET);

        if (ls < 0)
        {

            printf("Error During making the pointer point to location!!!");

            exit(1);
        }
        off_t red = read(fd, phdr, sizeof(Elf32_Phdr));
        if (red <= 0)
        {

            printf("Error while reading the file!!!");

            exit(1);
        }

        // printf("%d   ",phdr->p_memsz);
        // printf("%d :  ",ehdr->e_entry);
        // printf("%d     ",phdr->p_memsz+phdr->p_vaddr);
        if (phdr->p_type != PT_LOAD)
        {
            continue;
        }
        else
        {
            if (phdr->p_type == PT_LOAD && (phdr->p_vaddr <= ehdr->e_entry) && (ehdr->e_entry <= phdr->p_memsz + phdr->p_vaddr))
            {
                void *virtual_mem = mmap((void *)phdr->p_vaddr, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

                if (virtual_mem == MAP_FAILED)
                {
                    printf("Error  While Mapping Segment To Memory!!!");
                    exit(1);
                }
                if (lseek(fd, phdr->p_offset, SEEK_SET) == -1 ||
                    read(fd, virtual_mem, phdr->p_memsz) != phdr->p_memsz)
                {

                    printf("Error While Reading Segment Data!!!");

                    exit(1);
                }

                // Jump to entry point

                int (*_start)() = (int (*)())ehdr->e_entry;
                int result = _start();

                printf("User _start return value= %d", result);
            }
            else
            {
                continue;
            }
        }
    }
}

// printf("%d",fd);

// 1. Load entire binary content into the memory from the ELF file.
// 2. Iterate through the PHDR table and find the section of PT_LOAD
//    type that contains the address of the entrypoint method in fib.c
// 3. Allocate memory of the size "p_memsz" using mmap function
//    and then copy the segment content
// 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
// 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
// 6. Call the "_start" method and print the value returned from the "_start"
// int result = _start();
// printf("User _start return value = %d\n",result);

// MAIN FUNCTION

int main(int argc, char **argv)
{
    if (argc != 2)
    {

        printf("Usage: %s <ELF Executable> \n", argv[0]);

        exit(1);
    }
    load_and_run_elf(argv);
    loader_cleanup();
    return 0;
}

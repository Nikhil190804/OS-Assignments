#include "SimpleSmartLoader.h"
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Global variables for storing ELF header and program header
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;

// Global variables for various attributes of the loaded ELF file
Elf32_Addr virtual_address; // Virtual address for a segment
Elf32_Word mem_size;        // Memory size of a segment
Elf32_Off segment_offset;   // Offset of a segment in the ELF file
Elf32_Addr entry_point;     // Entry point address of the ELF binary

int fd; // File descriptor for the ELF file

long int total_page_faults = 0;            // Total page faults encountered
long int total_page_allocated = 0;         // Total pages allocated for segments
long int total_internal_fragmentation = 0; // Total internal fragmentation in bytes
char *elf_file;                            // File path of the ELF binary

// Function to calculate the number of pages needed for memory allocation
// declarations of functions
int number_of_pages_to_be_allocated(Elf32_Word mem_size_segment);
Elf32_Off find_segment_offset(Elf32_Addr seg_addr);
int find_segment_index(Elf32_Addr seg_addr);
void loader_cleanup();
void load_and_run_elf(char **exe);
void handle_temp_page_fault(siginfo_t *info);
void calculate_internal_frag();

// info for various segments
int insertion_index = 0;
struct Fault_Segment_Data
{
    Elf32_Off segment_offset;
    Elf32_Addr segment_address;
    Elf32_Word segment_size;
};
// array for segment data
struct Fault_Segment_Data fault_segment_array[1000];

void loader_cleanup()
{
    if (sizeof(ehdr) >= 0)
    {
        free(ehdr); // Free memory allocated for ELF header
    }
    if (sizeof(phdr) >= 0)
    {
        free(phdr); // Free memory allocated for program header
    }
    // Close the ELF file
    if (close(fd) == -1)
    {
        printf("Closing file...");
        exit(0);
    }
}

void load_and_run_elf(char **exe)
{
    elf_file = exe[1];
    fd = open(elf_file, O_RDONLY);
    if (fd < 0)
    {
        printf("Error in File Descriptor!!!\n");
        exit(1);
    }

    // Read ELF header
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    ssize_t reader = read(fd, ehdr, sizeof(Elf32_Ehdr));

    if (reader <= 0)
    {
        printf("Error while reading the file!!!\n");
        exit(1);
    }
    Elf32_Phdr *phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));

    if (ehdr->e_phnum == 0)
    {
        printf("Error in e_phnum!!!\n");
        exit(1);
    }
    entry_point = ehdr->e_entry;

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        // Move to program header entry
        off_t ls = lseek(fd, ehdr->e_phoff + i * sizeof(Elf32_Phdr), SEEK_SET);

        if (ls < 0)
        {
            printf("Error During making the pointer point to location!!!\n");
            exit(1);
        }
        off_t red = read(fd, phdr, sizeof(Elf32_Phdr));
        if (red <= 0)
        {
            printf("Error while reading the file!!!\n");
            exit(1);
        }
        /*
        printf("-----------\n");
        printf("segment type: %d\n", phdr->p_type);
        printf("segment flag: %d\n", phdr->p_flags);
        printf("-----------\n");*/
        virtual_address = phdr->p_vaddr;
        mem_size = phdr->p_memsz;
        // printf("mem size:%d \n", mem_size);
        segment_offset = phdr->p_offset;
        if (phdr->p_type == PT_LOAD && (phdr->p_vaddr <= ehdr->e_entry) && (ehdr->e_entry <= phdr->p_memsz + phdr->p_vaddr))
        {
            printf("Now Executing main (_start) method.....\n");
            int (*_start)() = (int (*)())ehdr->e_entry;
            int result = _start();
            printf("User _start return value= %d\n", result);
        }
        else
        {
            continue;
        }
    }
}

void handle_temp_page_fault(siginfo_t *info)
{
    Elf32_Addr fault_address = (Elf32_Addr)(uintptr_t)info->si_addr;
    Elf32_Addr masked_fault_address = fault_address & 0xFFFFF000;
    total_page_faults++;
    //printf("165\n");
    // search for that segment
    int file = open(elf_file, O_RDONLY);
    Elf32_Ehdr *new_ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    ssize_t reader = read(file, new_ehdr, sizeof(Elf32_Ehdr));
    Elf32_Phdr *new_phdr = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));
    for (int i = 0; i < new_ehdr->e_phnum; i++)
    {
        //printf("122\n");
        off_t ls = lseek(file, new_ehdr->e_phoff + i * sizeof(Elf32_Phdr), SEEK_SET);
        off_t red = read(file, new_phdr, sizeof(Elf32_Phdr));
        if (ls < 0 || red < 0)
        {
            printf("Error while seeking pointer and reading data!!!!!\n");
            exit(1);
        }
        if (new_phdr->p_vaddr + new_phdr->p_memsz > fault_address && fault_address >= new_phdr->p_vaddr)
        {
            // valid segment

            Elf32_Addr new_virtual_address = new_phdr->p_vaddr;
            Elf32_Word new_mem_size = new_phdr->p_memsz;
            Elf32_Off new_offset = find_segment_offset(new_virtual_address);
            Elf32_Off new_segment_offset = new_phdr->p_offset;
            //printf("mera :%d..\n", new_offset);
            //printf("real: %d\n", new_segment_offset);
            // this is insertion part only
            if(new_offset==-1){
                fault_segment_array[insertion_index].segment_address=new_virtual_address;
                fault_segment_array[insertion_index].segment_size=new_mem_size;
                fault_segment_array[insertion_index].segment_offset =1;
                insertion_index++;
            }
            else{
                //repeating segment ignore it
                int index = find_segment_index(new_virtual_address);
                fault_segment_array[index].segment_offset = (new_offset) + 1;
            }
            /*
            if (new_offset == -1)
            {
                // perform adding it to fault segment array
                fault_segment_array[insertion_index].segment_address = new_virtual_address;
                fault_segment_array[insertion_index].segment_offset = (new_phdr->p_offset) + 4096;
                insertion_index++;
            }
            else
            {
                new_segment_offset = new_offset;
                int index = find_segment_index(new_virtual_address);
                fault_segment_array[index].segment_offset = (new_offset) + 4096;
            }
            // int new_pages = number_of_pages_to_be_allocated(new_mem_size);
            // size_t new_memory = new_pages * 4096;
            // size_t difference = new_memory - new_mem_size;
            // total_internal_fragmentation += difference;*/
            total_page_allocated++;
            void *virtual_mem = mmap((void *)masked_fault_address, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
            if (virtual_mem == MAP_FAILED)
            {
                printf("Error  While Mapping Segment To Memory!!!\n");
                exit(1);
            }
            //printf("173\n");
            if (lseek(fd, new_segment_offset, SEEK_SET) == -1)
            {
                printf("Error While Reading Segment Data!!!\n");
                exit(1);
            }
            int a = read(fd, virtual_mem, 4096);
            if (a == 0)
            {
                printf("EOF reached!!!!!\n");
                exit(1);
            }
            return;
        }
    }
    printf("Not Found!!!\n");
    exit(1);
}

// Function to calculate the number of pages needed for memory allocation
int number_of_pages_to_be_allocated(Elf32_Word mem_size_segment)
{
    int number_of_pages = 0;
    if (mem_size_segment % 4096 == 0)
    {
        number_of_pages = mem_size_segment / 4096;
    }
    else
    {
        int quotient = mem_size_segment / 4096;
        number_of_pages = quotient + 1;
    }
    total_page_allocated += number_of_pages;
    return number_of_pages;
}

void segfault_handler(int signum, siginfo_t *info, void *context)
{
    // Check if the fault was due to a read or write access
    if (info->si_code == SEGV_MAPERR)
    {
        printf("Segmentation fault at address: %p\n", info->si_addr);
        handle_temp_page_fault(info);
    }
    else
    {
        printf("Segmentation fault of unknown cause\n");
    }
}


//function to find the offset of a segment for mmaping the memory
Elf32_Off find_segment_offset(Elf32_Addr seg_addr)
{
    Elf32_Off result = -1;
    for (int i = 0; i < insertion_index; i++)
    {
        if (fault_segment_array[i].segment_address == seg_addr)
        {
            result = fault_segment_array[i].segment_offset;
            return result;
        }
    }
    return result;
}

//function to find the index of segment in segment array
int find_segment_index(Elf32_Addr seg_addr)
{
    Elf32_Off result = -1;
    for (int i = 0; i < insertion_index; i++)
    {
        if (fault_segment_array[i].segment_address == seg_addr)
        {
            result = i;
            return result;
        }
    }
}

//function to calculate internal fragmentation using the array
void calculate_internal_frag(){
    long int total_page_alloc = total_page_allocated;
    long int total_mem_given = total_page_alloc*4096;
    long int actual_segment_memory=0;
    for(int i=0;i<insertion_index;i++){
        //printf("%d\n",fault_segment_array[i].segment_address);
        actual_segment_memory+=fault_segment_array[i].segment_size;
    }
    long int difference = total_mem_given-actual_segment_memory;
    //printf("%ld..\n",actual_segment_memory);
    //printf("%ld.\n",total_mem_given);
    total_internal_fragmentation=difference;
}

//main here
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Provide Correct Arguments!!!!! : %s \n", argv[0]);
        exit(1);
    }
    struct sigaction sa;
    sa.sa_sigaction = segfault_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
    // signal(SIGSEGV, handle_temp_page_fault);
    load_and_run_elf(argv);
    loader_cleanup();
    calculate_internal_frag();
    printf("\nTotal Page faults: %ld\n", total_page_faults);
    printf("\nPage Allocations: %ld\n", total_page_allocated);
    printf("\nTotal Internal Fragmentation: %ld bytes\n", total_internal_fragmentation);
    return 0;
}
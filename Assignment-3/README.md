## Overview
This project consists of two C programs: a scheduler and a shell. The scheduler is responsible for managing the execution of various tasks, and the shell serves as an interface for interacting with the scheduler. This README provides an overview of the project and instructions on how to compile, run, and use the programs.

## Scheduler
The scheduler program is responsible for managing the execution of tasks or processes. It uses a scheduling algorithm to determine the order in which tasks are executed. Details about the scheduling algorithm used, data structures, and program flow can be found in the design document (design.md).

### Compilation and Execution
To compile the scheduler program, use the following command:

```bash
gcc scheduler.c -o scheduler
```

To run the scheduler program, use the following command:

```bash
./scheduler <num_of_cpu> <time_slice>
```

## Shell
The shell program serves as an interactive interface for users to submit tasks to the scheduler, monitor task status, and perform other related actions. Details about the shell's features, supported commands, and program flow can be found in the design document (design.md).

### Compilation and Execution
To compile the shell program, use the following command:

```bash
gcc shell.c -o shell
```

To run the shell program, use the following command:

```bash
./shell <num_of_cpu> <time_slice>
```

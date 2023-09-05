//list all the libs used
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>



void ctrl_c_pressed(){
    // call the necessary func here for cleanup and termination 
    printf("\n now ending it ");
    exit(EXIT_SUCCESS);
}



//main func goes here
int main(){
    signal(SIGINT, ctrl_c_pressed);     //process the ctrl+c here 
    bool start_loop=true;
    char input[1000];
    printf("Welcome to Custom C Shell.....");
    while(start_loop){
        printf("\n"); 
        printf(">> : ");
        fgets(input,sizeof(input),stdin);
        size_t input_length = strlen(input);
        if (input_length > 0 && input[input_length - 1] == '\n') {
            input[input_length - 1] = '\0';
        }
        // Process the user input
        printf("You entered: %s\n", input);
    }
    return 0;
}

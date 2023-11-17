#include <stdio.h>
#include <time.h>

unsigned long long fib(int n) {
    if (n<2) return n;
    else return fib(n - 2) + fib(n - 1);
}

int main(int argc, char* argv[]) 
{
    time_t start_time;
    time(&start_time);
    printf("P2 HAS STARTEEEEEEEEEEEEEEEEEED %ld\n", start_time);
    int n= 42;
    int a = fib(n);
    
    time_t start_time2;
    time(&start_time2);
    printf("p2: %d %ld\n",a,start_time2);
    //printf("Fib(%d) = %llu\n", n, fib(n));
    return 0;
}
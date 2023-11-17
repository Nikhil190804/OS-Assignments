#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <pthread.h>
#include <time.h>

int user_main(int argc, char **argv);
// function to provide ranges for parallel_for for one loop

double calculate_time_taken(clock_t s,clock_t e){
  double result;
  result= ((double)(e-s))/CLOCKS_PER_SEC * 1000.0;
  return result;
}

void print_details(double arg){
  printf("Call for Parallel_for finished.....\n");
  printf("Time Taken to execute: %f ms\n",arg);
}

int find_proper_chunk(int low, int high, int thread_count)
{
  int result;
  if (high % 2 == 0)
  {
    // even here
    result = (high - low) / thread_count;
  }
  else
  {
    result = ((high - low) + 1) / thread_count;
  }
  return result;
}

bool has_extra_index(int low, int high, int thread_count)
{
  bool result;
  int remainder = ((high - low) + 1) % thread_count;
  if (remainder != 0)
  {
    result = true;
  }
  else
  {
    result = false;
  }
  return result;
}

// parallel_for accepts a C++11 lambda function and runs the loop body (lambda) in
// parallel by using ‘numThreads’ number of Pthreads to be created by the simple-multithreader

// function for parallel_for for single loops
std::function<void(int)> Lambda_for_single_loop;
// global var for nested loops lambda fucntion
std::function<void(int, int)> Lambda_for_nested_loop;

void *parallel_for_single_loop(void *data)
{
  int *index_data = (int *)data;
  int start = index_data[0];
  int end = index_data[1];
  // printf("%d ..%d\n",start,end)
  for (int i = start; i < end; i++)
  {
    Lambda_for_single_loop(i);
    // localLambda(i);
  }
  // printf("%d ..%d done\n",start,end);
  free(data);
  return NULL;
}

void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads)
{
  clock_t starttime;
  clock_t endtime;
  starttime=clock();
  Lambda_for_single_loop = lambda;
  printf("Parallel_for Call Occured.....\n");
  int chunk_size = find_proper_chunk(low, high, numThreads);
  // printf("%d\n",chunk_size);
  pthread_t total_threads[numThreads];
  for (int i = 0; i < numThreads; i++)
  {
    int start = i * chunk_size;
    int end = start + chunk_size;
    if (i == numThreads - 1)
    {
      // last thread to execute
      // check for extra index
      bool valid = has_extra_index(low, high, numThreads);
      if (valid == true)
      {
        // has extra index
        end += ((high - low) + 1) % numThreads;
      }
    }
    // printf("%d %d\n",start,end);
    int *index_data = (int *)(malloc(2 * sizeof(int)));
    index_data[0] = start;
    index_data[1] = end;
    // now create a thread and pass it the arguments
    int thread_status=pthread_create(&total_threads[i], NULL, &parallel_for_single_loop, index_data);
    if(thread_status!=0){
      printf("Error while creating a thread!!!!!\n");
      exit(1);
    }
  }

  for (int i = 0; i < numThreads; i++)
  {
    int thread_status=pthread_join(total_threads[i], NULL);
    if(thread_status!=0){
      printf("Error while waiting for the thread to join to the main thread!!!!!\n");
      exit(1);
    }
  }
  endtime=clock();
  double answer = calculate_time_taken(starttime,endtime);
  print_details(answer);

}

// This version of parallel_for is for parallelizing two-dimensional for-loops, i.e., an outter for-i loop and
// an inner for-j loop. Loop properties, i.e. low, high are mentioned below for both outter
// and inner for-loops. The suffixes “1” and “2” represents outter and inner loop properties respectively.

void *parallel_for_nested_loop(void *data)
{
  int *index_data = (int *)data;
  int start = index_data[0];
  int end = index_data[1];
  int nested_start = index_data[2];
  int nested_end = index_data[3];
  // printf("%d.......%d\n",start,end);
  // Lambda_for_nested_loop(0,0);
  for (int i = start; i < end; i++)
  {
    for (int j = nested_start; j < nested_end; j++)
    {
      Lambda_for_nested_loop(i, j);
    }
  }
  free(data);
  return NULL;
}

void parallel_for(int low1, int high1, int low2, int high2, std::function<void(int, int)> &&lambda, int numThreads)
{
  clock_t starttime;
  clock_t endtime;
  starttime=clock();
  printf("Parallel_for Call Occured.....\n");
  Lambda_for_nested_loop = lambda;
  pthread_t total_threads[numThreads];
  
  // printf("%d\n",chunk_size);
  int chunk_size = find_proper_chunk(low1, high1, numThreads);
  for (int i = 0; i < numThreads; i++)
  {
    int start = i * chunk_size;
    int end = start + chunk_size;
    if (i == numThreads - 1)
    {
      // last thread to execute
      // check for extra index
      bool valid = has_extra_index(low1, high1, numThreads);
      if (valid == true)
      {
        // has extra index
        end += ((high1 - low1) + 1) % numThreads;
      }
    }
    int nested_start = low2;
    int nested_end = high2;
    //printf("%d..%d\n", start, end);
    int *index_data = (int *)(malloc(4 * sizeof(int)));
    index_data[0] = start;
    index_data[1] = end;
    index_data[2] = nested_start;
    index_data[3] = nested_end;
    int thread_status=pthread_create(&total_threads[i], NULL, &parallel_for_nested_loop, index_data);
    if(thread_status!=0){
      //error has happened 
      printf("Error while creating a thread!!!!!\n");
      exit(1);
    }
  }
  for (int i = 0; i < numThreads; i++)
  {
    int thread_status=pthread_join(total_threads[i], NULL);
    if(thread_status!=0){
      //error has occurred
      printf("Error while waiting for the thread to join to the main thread!!!!!\n");
      exit(1);
    }
  }
  endtime=clock();
  double answer = calculate_time_taken(starttime,endtime);
  print_details(answer);
}

/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> &&lambda)
{
  lambda();
}

int main(int argc, char **argv)
{
  /*
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be
   * explicity captured if they are used inside lambda.
   */
  int x = 5, y = 1;
  // Declaring a lambda expression that accepts void type parameter
  auto /*name*/ lambda1 = /*capture list*/ [/*by value*/ x, /*by reference*/ &y](void)
  {
    /* Any changes to 'x' will throw compilation error as x is captured by value */
    y = 5;
    std::cout << "====== Welcome to Assignment-" << y << " of the CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);

  auto /*name*/ lambda2 = [/*nothing captured*/]()
  {
    std::cout << "====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

#define main user_main

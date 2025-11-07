#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

#define NTHREADS 3
#define MAX_SIZE 1000 		// How many to read
#define FILENAME "numbers.txt"

void* check_primes();

void printResults();

// Global variables (accessible by all threads)
int numbers[MAX_SIZE];           // will be read from file
int primesFound[MAX_SIZE];       // will hold the primes that are found
int primesFoundCount = 0;        // how many primes found so far??

int main()
{
  int i;
  
  // Ensure memory is empty to start
  for (i=0; i<MAX_SIZE; i++) {
    primesFound[i] = 0;
  }
  
  // Read numbers from file, store into array
  FILE * file = fopen (FILENAME, "r");
  if (file == NULL) {
    printf("Error opening %s\n", FILENAME);
    exit(1);
  }
  i = 0;
  while ((!feof(file)) && (i < MAX_SIZE)) {  
    fscanf (file, "%d", &numbers[i]);    
    i++;
  }
  fclose (file);        

  // TODO: remove or comment out the next line for the multi-threaded version:
  check_primes();
  
  // TODO: create the threads, and have each one execute 'check_primes'
  
  // TODO: join threads when complete

  // Print the results
  printf ("Found a total of %d primes.\n", primesFoundCount);
  // TODO (later!): uncomment printResults() when instructed to do so
  //printResults();  
}

// Print out all the primes that were found
void printResults() {
  int i;
  for (i=0; i<primesFoundCount; i++) {
    printf("%d ", primesFound[i]);
  }
  printf ("\n");
}

// Returns true if 'num' is prime
bool isPrime(int num) {
  if (num <= 1) return false;
  for (int j = 2; j < num; j++) {
    if (num % j == 0) {
      return false;
    }
  }
  return true;  // no factor found, so must be prime
}

// Check all values in the array numbers[], and place any primes found in the primesFound[] array.
void* check_primes()
{
  // TODO: make any necessary changes so that each thread can call this function AND
  //   have the 'work' divided evenly among the threads.
  //   Do NOT make multiple copies of any code!
  //   Also, your code must work work any value of NTHREADS
  
  int i   = 0;
  int num = 0;

  for (i=0; i<MAX_SIZE; i++) {
    num = numbers[i];
    if (isPrime(num)) {
      primesFound[primesFoundCount] = num;
      printf("found %d \n",num);
      primesFoundCount++;
    }
  }
  return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define NTHREADS 20
#define SLEEPY_THREAD 3
#define NAP_TIME 2
void *thread_function(void *);

int main()
{
  pthread_t thread_id[NTHREADS];
  int i;
  long t_num;  // must be 'long' vice int to make easier to pass as arg to pthread_create
   
  // Create the threads, passing 't_num' as argument to thread (becomes the thread number)
  for(t_num=0; t_num < NTHREADS; t_num++) {
    pthread_create( &thread_id[t_num], NULL, thread_function, (void*) t_num );
  }

  // Join all threads when complete
  for(i=0; i < NTHREADS; i++) {
    pthread_join( thread_id[i], NULL);
  }
}

void *thread_function(void * t_num)
{
  int i = 0;
  long thread_num = (long) t_num;   // convert passed argument, to treat as an int
  if (thread_num == SLEEPY_THREAD) {
    sleep(NAP_TIME);
  }
  printf("Thread number:");    
  for (i=0;i<thread_num;i++) {
    printf(" ");
  }
  printf("%ld finished\n", thread_num);
  return 0;
}

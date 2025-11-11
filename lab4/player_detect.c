#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define NUM_PLAYERS			24
#define NUM_BALLS			8
#define NUM_BATS			8
#define NUM_GLOVES			8
#define RESOURCE_COUNT		3
#define SHORT_WAIT			.01
#define PLAY				1
#define BALL				0
#define BAT					1
#define GLOVE				2

/*==========================================================================*/
					/* No changes to this section */

/* Macros to encapsulate POSIX semaphore functions so they look like notes. Examples:
 * semaphore my_sem;
 * semInit(my_sem,1);
 * semWait(my_sem);
 * semSignal(my_sem);
 * semRelease(my_sem);
 * */
#define semInit(s,v)	sem_init( &s, 0, v )
#define semWait(s)		sem_wait( &s )
#define semSignal(s)	sem_post( &s )
#define semRelease(s)	sem_destroy( &s )
#define semValue(s,v)	sem_getvalue( &s, &v )

typedef sem_t semaphore;

/* Semaphores to control allocation of shared resources */
semaphore balls_sem;
semaphore bats_sem;
semaphore gloves_sem;

pthread_t player[NUM_PLAYERS];

/* Data structures to track shared resources. */
/* The number in each array indicates the number of the thread owning the resource. When not in use, set to -1. */
int ball_owner[NUM_BALLS];
int bat_owner[NUM_BATS];
int glove_owner[NUM_GLOVES];
/* The number in each array represents the number of the ball, bat, or glove held by that player thread. When not in use, set to -1. */
int my_ball[NUM_PLAYERS];
int my_bat[NUM_PLAYERS];
int my_glove[NUM_PLAYERS];
bool is_done[NUM_PLAYERS];

/* Boolean to indicate all players have completed */
bool alldone = false;

/* Deadlock detection data structures */
int R[NUM_PLAYERS][RESOURCE_COUNT] = {{0}};
int C[NUM_PLAYERS][RESOURCE_COUNT] = {{0}};
int E[RESOURCE_COUNT] = {NUM_BALLS, NUM_BATS, NUM_GLOVES};
int A[RESOURCE_COUNT] = {0};
int A0[RESOURCE_COUNT] = {0};    // backup copy of A[] (for printing later)

/*==========================================================================*/
					/* No changes to this section */

int get_ball(int n) {
	int i = 0;
	semWait(balls_sem);	// Wait for a ball
	while (ball_owner[i] != -1) i = (i+1) % NUM_BALLS; // Choose a free one
	my_ball[n] = i;
	ball_owner[i] = n;
	printf("player %i got ball %i\n",n, i); fflush(stdout);
	R[n][BALL] = 0;
	C[n][BALL]= 1;	
	return i;
}

int get_bat(int n) {
	int i = 0;
	semWait(bats_sem); // Wait for a bat
	while (bat_owner[i] != -1) i = (i+1) % NUM_BATS; // Choose a free one
	my_bat[n] = i;
	bat_owner[i] = n;
	printf("player %i got bat %i\n",n, i); fflush(stdout);
	R[n][BAT] = 0;
	C[n][BAT]= 1;	
	return i;
}

int get_glove(int n) {
	int i = 0;
	semWait(gloves_sem); // Wait for a glove
	while (glove_owner[i] != -1) i = (i+1) % NUM_GLOVES; // Choose a free one	
	my_glove[n] = i;
	glove_owner[i] = n;
	printf("player %i got glove %i\n",n, i); fflush(stdout);
	R[n][GLOVE] = 0;
	C[n][GLOVE]= 1;	
	return i;
}

int drop_ball(int n) {
	int ball_num = my_ball[n];
	ball_owner[ball_num] = -1;
	my_ball[n] = -1;
	C[n][BALL]= 0;	
	semSignal(balls_sem);
	return ball_num;
}

int drop_bat(int n) {
	int bat_num = my_bat[n];
	bat_owner[bat_num] = -1;
	my_bat[n] = -1;
	C[n][BAT]= 0;	
	semSignal(bats_sem);
	return bat_num;
}

int drop_glove(int n) {
	int glove_num = my_glove[n];
	glove_owner[glove_num] = -1;
	my_glove[n] = -1;
	C[n][GLOVE]= 0;	
	semSignal(gloves_sem);
	return glove_num;
}

/*==========================================================================*/
					/* No changes to this section */

void * player_function(void * t_num)
/* Simulate a player. Each player must obtain a ball, bat, and glove before playing. 
This function is called via pthread_create(). Player threads randomly choose 
which resource to seek first. */
{
    int n = * (int *) t_num;
    R[n][BALL] = 1; R[n][BAT] = 1; R[n][GLOVE] = 1; // Update request matrix R;    
    
    int r = rand() % 3;		/* Get Supplies */
    if (r == 0) {
		get_ball(n); sleep(SHORT_WAIT);
		get_bat(n); sleep(SHORT_WAIT);
		get_glove(n); sleep(SHORT_WAIT);	
	}
	else if (r == 1) {
		get_bat(n); sleep(SHORT_WAIT);
		get_glove(n); sleep(SHORT_WAIT);		
		get_ball(n); sleep(SHORT_WAIT);
	}
	else {
		get_glove(n); sleep(SHORT_WAIT);
		get_ball(n); sleep(SHORT_WAIT);
		get_bat(n); sleep(SHORT_WAIT);
	}

	sleep(PLAY);

	/* Release Supplies  and finish */
	printf("player %i released glove %i\n", n, drop_glove(n)); fflush(stdout);
	printf("player %i released bat %i\n", n, drop_bat(n)); fflush(stdout);
	printf("player %i released ball %i\n", n, drop_ball(n)); fflush(stdout);
	printf("player %i ........... done\n", n); fflush(stdout); 
	is_done[n] = true;
	return 0;
}

/*==========================================================================*/
void * deadlock_detector(void * unused_argument)
{
	bool deadlock = false;
	bool all_zero, can_run, some_just_ran;
	bool deadlocked[NUM_PLAYERS] = {false};
	bool ran[NUM_PLAYERS] = {false};
	int R_snapshot[NUM_PLAYERS][RESOURCE_COUNT] = {{0}};
	int C_snapshot[NUM_PLAYERS][RESOURCE_COUNT] = {{0}};
	int i,j,sum;
	
	while (!(deadlock || alldone)) {
		
		// Snapshot (make a copy) of the requests and allocations
		// (we do this because we want to have a copy of these values that is "stable" while the detection algorithm runs below -- and because C[] and R[] are still in use by the main threads)
		memcpy(R_snapshot, R, sizeof (int) * NUM_PLAYERS * RESOURCE_COUNT);
		memcpy(C_snapshot, C, sizeof (int) * NUM_PLAYERS * RESOURCE_COUNT);

		// Populate A[] matrix
		for (i=0; i < RESOURCE_COUNT; i++) {
			sum = 0;
			for (j=0; j < NUM_PLAYERS; j++) {
				sum += C_snapshot[j][i];
			}
			A[i] = E[i] - sum;
		}
		
		// Make backup copy of A[] into A0[] (so that we can print a copy of *original* values of A, via A0, if deadlock is found
		memcpy(A0, A, sizeof (int) * RESOURCE_COUNT);

		// Reset per-thread indicators
		for (i=0; i<NUM_PLAYERS; i++) ran[i] = false;
		
		// Look for a thread that can run, and (notionally) run it. 
		// Repeat until nothing runs.
		some_just_ran = true;
		while (some_just_ran) {
			
			some_just_ran = false;
			for (i=0; i<NUM_PLAYERS; i++) {

				if ((C_snapshot[i][BALL]==0) && 
					(C_snapshot[i][GLOVE]==0) && 
					(C_snapshot[i][BAT]==0))
				{
					// Thread holds nothing, so it can't be deadlocked
					deadlocked[i] = false;
				}

				// If thread not done, see if resources are available for it to run
				else if (!(is_done[i] || ran[i])) {

					can_run = true;   
<<<<<<< HEAD
					for (int j = 0; j < RESOURCE_COUNT; j++)
					{
						if (R_snapshot[i][j] > A[j])		//check if resource requested is greater than resource open
						{
							can_run = false;
							break;
						}
					}
=======
					
>>>>>>> d3991a2127cf2398032fa436b686c0ca42d52195
					/* TODO:  For thread i, you must now loop over all resources.
					   If, for any resource, there are more requested in R_snapshot (for this thread) than available in A, then thread i can't "run",
					   and so you must set can_run to false. */
					
					/* write some code here!!! (based on the TODO above) */

					// If 'can_run' got changed to false above, then note that this thread is potentially deadlocked
					// (it's only "potential" at this point, because perhaps other threads will run and release some resources.
					if (can_run == false) {
					    deadlocked[i] = true;
					}
					else {
					  // Otherwise, can_run must be true....

					  /* TODO: notionally 'run' thread i. Specifically, free its resources
					     by *moving* them (for each possible resource) from the appropriate part of the 
					     Claims matrix (C_snapshot) to the Available vector (A).
					     Note: we must modify C_snapshot[], *not* C[] because C[] is still in use by the actual running threads.  A[] is okay to modify because it
					     is only used by the deadlock detector thread. 
					  */

					  /* write some code here!!! (based on the TODO above) */
					  
<<<<<<< HEAD
							for (int j = 0; j < RESOURCE_COUNT; j++)
							{
								A[j] = A[j] + C_snapshot[i][j];
								C_snapshot[i][j] = 0;
							}

=======
>>>>>>> d3991a2127cf2398032fa436b686c0ca42d52195
					  // After freeing the resources, remember that this thread has run, and is therefore not currently deadlocked,
					  ran[i]        = true;	  // Thread i had enough resources to notionally run.
					  deadlocked[i] = false;
					  // ... and remember that at least one thread just made some progress (important below)
					  some_just_ran = true;
									            
					}
				}				
			}
			
			// Update the 'deadlock' variable that is used to control the loop:
			//   Do some individual threads remain unable to run?  
 			//   If yes, and nothing ran this time, we have deadlocked (but ignore threads that are already done)
			//   The threads still unable to run are the deadlock set.
			if (!some_just_ran) {
				deadlock = false;
				for (i=0; i<NUM_PLAYERS; i++) {
					if (deadlocked[i] && !is_done[i]) deadlock = true;
				}
			}
			
		} // End 'while (some_just_ran) ' 

		
		// Print a report
                if (!deadlock){
			printf("\t\t\t\t**No deadlock**\n");
                }                  
		else {
			printf("\t\t\t\t**Deadlock detected!**\n");
		
			/* print C and R matrix snapshots */
			printf("             C        R\n");
			for (i=0; i<NUM_PLAYERS; i++) {
				if (deadlocked[i]) {
					printf("thread %2i: ",i);
					for (j=0; j<RESOURCE_COUNT; j++) printf("%1i ",C_snapshot[i][j]);
					printf("   ");				
					for (j=0; j<RESOURCE_COUNT; j++) printf("%1i ",R_snapshot[i][j]);
					printf("\n");
				}
<<<<<<< HEAD

=======
>>>>>>> d3991a2127cf2398032fa436b686c0ca42d52195
			}
			
			/* Print E, A vectors.  Use A0 so that we get the *original* value of A[] */
			printf("\nE: ");
			for (i=0; i<RESOURCE_COUNT; i++) printf("%1i ",E[i]);
			printf("   A: ");
			for (i=0; i<RESOURCE_COUNT; i++) printf("%1i ",A0[i]);
			printf("\n\n");

			/* TODO: *not* required, but if needed, the following functions can be un-commented for debugging */

			/* print semaphore values
			int v;
			semValue(balls_sem,v); printf("\nBalls_sem: %i", v);
			semValue(bats_sem,v); printf("\nBats_sem: %i", v);
			semValue(gloves_sem,v); printf("\nGloves_sem: %i", v);	
			printf("\n"); */						

			/* print by-thread deadlock values
			printf("Deadlocked Players: ");
			for (i=0; i<NUM_PLAYERS; i++) {
				if (deadlocked[i]) { printf("%i ",i); }
			}
			printf("\n"); */

			/* print by-thread finished values
			printf("Finished Players: ");
			for (i=0; i<NUM_PLAYERS; i++) {
				if (is_done[i]) { printf("%i ",i); }
			} */

			/* Print who has what
			printf("\nBalls: ");
			for (i=0; i<NUM_BALLS; i++) printf("%i ",ball_owner[i]);
			printf("\nBats: ");
			for (i=0; i<NUM_BATS; i++) printf("%i ",bat_owner[i]);
			printf("\nGloves: ");
			for (i=0; i<NUM_GLOVES; i++) printf("%i ",glove_owner[i]);
			printf("\n"); */
		}
		
		sleep(PLAY);
		
	} // End 'while !deadlock'
	return 0;
}

/*==========================================================================*/

int main( int argc, char *argv[] )
{
    int i;
    pthread_t deadlock_detect_thread;    
    int statics[NUM_PLAYERS];
    srand(time(NULL));

    /* Initialize semaphores that block threads to avoid multiple allocation  */
    semInit(balls_sem, NUM_BALLS);
    semInit(bats_sem, NUM_BATS);
    semInit(gloves_sem, NUM_GLOVES);

    /* Initially all resources are free (set to -1) */
    for (i=0; i<NUM_BALLS; i++) ball_owner[i] = -1;
    for (i=0; i<NUM_BATS; i++) bat_owner[i] = -1;
    for (i=0; i<NUM_GLOVES; i++) glove_owner[i] = -1;    	

    /* Initialize player data. Create a thread for each player. */
    for ( i = 0; i < NUM_PLAYERS; i++ )
    {
		statics[i] = i;
		my_ball[i] = -1; my_bat[i] = -1; my_glove[i] = -1;
		pthread_create( &player[i], NULL, player_function, &statics[i] ); 
		is_done[i] = false;
    }

    /* TODO: Create deadlock detection thread */
<<<<<<< HEAD
	pthread_create(&deadlock_detect_thread , NULL, deadlock_detector, NULL);
=======

>>>>>>> d3991a2127cf2398032fa436b686c0ca42d52195

    /* Wait for all players to finish. */
    for ( i = 0; i < NUM_PLAYERS; i++ ) pthread_join( player[i], NULL );
    alldone = true;

    /* TODO: Wait for detector to finish. */
<<<<<<< HEAD
	pthread_join( deadlock_detect_thread, NULL);
=======

>>>>>>> d3991a2127cf2398032fa436b686c0ca42d52195

    /* Release resource blocking semaphores */
    semRelease(balls_sem);
    semRelease(bats_sem);
    semRelease(gloves_sem);

    /* Produce the final report. */
    printf( "All players have played ball!\n" );
    return 0;
}

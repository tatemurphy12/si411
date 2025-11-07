#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define NUM_PLAYERS			24
#define NUM_BALLS			6
#define NUM_BATS			6
#define NUM_GLOVES			6
#define RESOURCE_COUNT		3
#define SHORT_WAIT			.01
#define PLAY				1
#define BALL				0
#define BAT					1
#define GLOVE				2

/*==========================================================================*/

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
//int r = 0;

/* Boolean to indicate all players have completed */
bool alldone = false;

/* Deadlock detection data structures */
int R[NUM_PLAYERS][RESOURCE_COUNT] = {{0}};
int C[NUM_PLAYERS][RESOURCE_COUNT] = {{0}};
int E[RESOURCE_COUNT] = {NUM_BALLS, NUM_BATS, NUM_GLOVES};
int A0[RESOURCE_COUNT] = {0};
int A[RESOURCE_COUNT] = {0};

/*==========================================================================*/

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
        //r++;
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

    /* Wait for all players to finish. */
    for ( i = 0; i < NUM_PLAYERS; i++ ) pthread_join( player[i], NULL );
    alldone = true;


    /* Release resource blocking semaphores */
    semRelease(balls_sem);
    semRelease(bats_sem);
    semRelease(gloves_sem);

    /* Produce the final report. */
    printf( "All players have played ball!\n" );
    return 0;
}

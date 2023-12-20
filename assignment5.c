#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "random.h"

// -- MACROS FOR CONFIGURATION --
// for submission: 5, 100, 1, 9, 3, 1, 11, 7
#define philosophers 5
#define totalEatingTime 100
#define minimumEatingTime 1
#define eatingMean 9
#define eatingSD 3
#define minimumThinkingTime 1
#define thinkingMean 11
#define thinkingSD 7

pid_t wait(int *wstatus);

/* successive calls to randomGaussian produce integer return values */
/* having a gaussian distribution with the given mean and standard  */
/* deviation.  Return values may be negative.                       */

int randomGaussian(int mean, int stddev) {
	double mu = 0.5 + (double) mean;
	double sigma = fabs((double) stddev);
	double f1 = sqrt(-2.0 * log((double) rand() / (double) RAND_MAX));
	double f2 = 2.0 * 3.14159265359 * (double) rand() / (double) RAND_MAX;
	if (rand() & (1 << 5)) 
		return (int) floor(mu + sigma * cos(f2) * f1);
	else            
		return (int) floor(mu + sigma * sin(f2) * f1);
}

int macroCheck() {
	// only checking macro value
	if ((philosophers <= 0) || 
		(totalEatingTime <= 0) || 
		(minimumEatingTime <= 0) || 
		(minimumThinkingTime <= 0)){
		return -1;
	} else {
		return 1;
	}
}

int main(){

	if (macroCheck() == -1) {
		printf("Macros are set incorreclty\n");
		return 0;
	}

	// going for the array of semephores approach
	int semID = semget(IPC_PRIVATE, philosophers, IPC_CREAT | IPC_EXCL | 0600);
	
	if (semID < 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(0);
	}

	// init vars
	struct sembuf waitSem[2] = {{0, 1, 0}, {0, 1, 0}};
	struct sembuf signalSem[2] = {{0, -1, 0}, {0, -1, 0}};

	int eatTime, thinkTime, R, totalTime = 0;

	time_t t;
	
	for (int i = 0; i < philosophers; i++) {
		int cID = fork();
		if (cID == 0) {
			// child process

			srand(time(&t)+i); 

			if ((i+1) >= philosophers) {
				R = 0;
			} else {
				R = i+1;
			}

			// set chopsticks for each philosopher
			waitSem[0].sem_num = i;
			waitSem[1].sem_num = R;
			signalSem[0].sem_num = i;
			signalSem[1].sem_num = R;

			while (totalTime <= totalEatingTime) {

				// think logic
				thinkTime = randomGaussian(thinkingMean, thinkingSD);
				if (thinkTime < minimumThinkingTime) {
					thinkTime = minimumThinkingTime;
				}
				printf("Philosopher %d has started thinking\n", (i+1));
				sleep(thinkTime);
				printf("Philosopher %d thought for %d seconds\n", (i+1), thinkTime);

				// wait for both available chopsticks
				if (semop(semID, waitSem, 1) < 0) {
					fprintf(stderr, "%s\n", strerror(errno));
					exit(0);
				}

				// eat logic
				eatTime = randomGaussian(eatingMean, eatingSD);
				if (eatTime < minimumEatingTime) {
					eatTime = minimumEatingTime;
				}
				printf("Philosopher %d has started eating\n", (i+1));
				sleep(eatTime);
				totalTime += eatTime;
				printf("Philosopher %d ate for %d seconds\n", (i+1), eatTime);
				
				// put down chopsticks
				if (semop(semID, signalSem, 1) < 0) {
					fprintf(stderr, "%s\n", strerror(errno));
					exit(0);
				}

			}

			printf("Philosopher %d has finished eating\n", (i+1));
			exit(0);
		} else {
			if (cID == -1) {
				fprintf(stderr, "%s\n", strerror(errno));
				exit(0);
			}
		}
	}

	// parent process
	
	// waiting for child processes to end
	for (int i = 0; i < philosophers; i++) {
		wait(NULL);
	}

	// clean-up artifacts
	if (semctl(semID, 0, IPC_RMID) < 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(0);
	}

	printf("Philosophers are all done!\n");

	return 0;
}
/** producer.c for donuts process implementations **/

#include "donuts.h"
#include <stdlib.h>
#include <stdio.h>

int	shmid, semid[3];

void 	sig_handler(int);




int	main(int argc, char *argv[])
{

	int		in_ptr[NUMFLAVORS];
	int		serial[NUMFLAVORS];
	int		i,j,k,nsigs;
	struct donut_ring 	*shared_ring;
	struct timeval 		randtime;
	unsigned short 		xsub1[3];
	struct sigaction	new;
	sigset_t   		mask_sigs;
  	int sigs[] = {SIGHUP,SIGINT,SIGQUIT, SIGPIPE,
                 SIGTERM, SIGBUS, SIGSEGV, SIGFPE};

// set signal handler for likely signals

	nsigs = sizeof(sigs)/sizeof(int);

   	sigemptyset(&mask_sigs);

	for(i=0; i<NUMFLAVORS; i++){
		in_ptr[i]=0;
		serial[i]=1;
	}

   	for(i=0; i< nsigs; i++)
       	  	sigaddset(&mask_sigs, sigs[i]);

   	for(i=0; i< nsigs; i++){
      		new.sa_handler = sig_handler;
      		new.sa_mask = mask_sigs;
      		new.sa_flags = 0;
      		if(sigaction(sigs[i], &new, NULL) == -1){
         		perror("can't set signals: ");
         		exit(1);
		}
      	}

// allocate shared memory ID, store in global variable shmid and map shared memory

	if((shmid=shmget(MEMKEY, sizeof(struct donut_ring),
			IPC_CREAT | 0600)) == -1){
		perror("shared get failed: ");
		exit(1);
	}

	if((shared_ring=(struct donut_ring *)shmat(shmid, NULL, 0)) == (void *)-1){
		perror("shared attach failed: ");
		sig_handler(-1);
	}

// allocate 3 sem-IDs each with 4 counters, store in global array semid[]

	for(i=0; i<NUMSEMIDS; i++)
		if((semid[i]=semget(SEMKEY+i, NUMFLAVORS, 
			IPC_CREAT | 0600)) == -1){
		perror("semaphore allocation failed: ");
		sig_handler(-1);
	}

// prepare to seed the nrand48 random number generator

     	gettimeofday(&randtime, NULL);

        xsub1[0] = (ushort)randtime.tv_usec;
        xsub1[1] = (ushort)(randtime.tv_usec >> 16);
        xsub1[2] = (ushort)(getpid());

// use the utility function setsetall to set the 4 counters of each sem-ID

	if(semsetall(semid[PROD], NUMFLAVORS, NUMSLOTS) == -1){
		perror("semsetall failed: ");
		sig_handler(-1);
	}
	if(semsetall(semid[CONSUMER], NUMFLAVORS, 0) == -1){
                perror("semsetall failed: ");
                sig_handler(-1);
        }
	if(semsetall(semid[OUTPTR], NUMFLAVORS, 1) == -1){
                perror("semsetall failed: ");
                sig_handler(-1);
        }


	
	while(1){

// use the nrand48 function to pick a floavor of donut to produce

	  j=nrand48(xsub1) & 3;

// check for space in the selected flavor ring buffer

	  if( p(semid[PROD], j) == -1){
                perror("p operation failed: ");
                sig_handler(-1);
          }


// when space is available, get the donut number for selected flavor
// and place the number in the correct ring buffer element

	  shared_ring->flavor[j][in_ptr[j]]=serial[j];

// increment the selected in_ptr mod N and the donut number monotonically

	  in_ptr[j] = (in_ptr[j]+1) % NUMSLOTS;
	  serial[j]++;

// signal the consumer sem-ID selected flavor counter

	  if( v(semid[CONSUMER], j) == -1){
                perror("v operation failed: ");
                sig_handler(-1);
          }
	}
}  // forever


// use this signal handler to clean up

void	sig_handler(int sig)
{
	int	i,j,k;

	printf("In signal handler with signal # %d\n",sig);

	if(shmctl(shmid, IPC_RMID,0) == -1){
		perror("handler failed shm RMID: ");
	}

	for(i=0; i<NUMSEMIDS; i++){
	  if(semctl(semid[i], 0, IPC_RMID) == -1){
		perror("handler failed sem RMID: ");
	  }
	}
	exit(4);
}

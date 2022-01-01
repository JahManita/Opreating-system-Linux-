/** consdonuts.c for donuts process implementation **/

#include "donuts.h"
#include <stdlib.h>
#include <stdio.h>

int	shmid, semid[3];


int main(int argc, char *argv[])
{

	int	i,j,k,donut;
	struct donut_ring *shared_ring;
        struct timeval          randtime;
        unsigned short          xsub1[3];
	char *dtype[] = {"jelly" , "coconut" , "plain" , "glazed"};
        int doz[12][4];
        int d[4], m, n;
        time_t t;
        struct tm * tp;
        float ms;
        int ims;


// lookup the shm-ID (don't try to create it)

	if((shmid=shmget(MEMKEY, sizeof(struct donut_ring),
				0)) == -1){
		perror("shared get failed: ");
		exit(1);
	}

// map the shared memory segment

	if((shared_ring=(struct donut_ring *)shmat(shmid, NULL, 0)) ==(void *) -1){
		perror("shared attach failed: ");
		exit(1);
	}

// lookup the sem-IDs (don't try to create them)

	for(i=0; i<NUMSEMIDS; i++){
		if((semid[i]=semget(SEMKEY+i, NUMFLAVORS, 
			0)) == -1){
		perror("semaphore allocation failed: ");
		exit(1);
		}
	}

// prepare nrand48 random number generator

        gettimeofday(&randtime, NULL);

        xsub1[0] = (ushort)randtime.tv_usec;
        xsub1[1] = (ushort)(randtime.tv_usec >> 16);
        xsub1[2] = (ushort)(getpid());

// prepare for NUMDZ dozen run

	for(i=0; i<NUMDZ; i++){

// zero dozen array and flavor indecies

	 for(m=0; m<12; ++m)
	  for(n=0; n<4; ++n) doz[m][n]=0;
	 for(n=0; n<4; ++n) d[n]=0;
// get info for dozen header, including PID, time, doz number

	t=time(NULL);
	tp = localtime(&t);
	gettimeofday(&randtime, NULL);

// get fractional second value as float

	ms = (float)randtime.tv_usec/1000000;

// convert to 3 digit int for printf below

        ims = (int)(ms*1000);

// print dozen header to stdout

	printf("consumer PID %d   time: %d:%d:%d.%d   dozen number: %d  \n\n",
	        getpid(), tp->tm_hour, tp->tm_min, tp->tm_sec, ims, i+1);

// now get a dozen donuts

	   for(k=0; k<12; k++){

// select flavor to get

	  	j=nrand48(xsub1) & 3;

// wait for flavor to be in ring buffer if it's not yet

	  	if( p(semid[CONSUMER], j) == -1){
                	perror("p operation failed: ");
                	exit(3);
          	}

// locak the selected out_ptr for exclusive use

		if( p(semid[OUTPTR], j) == -1){
                        perror("p operation failed: ");
                        exit(3);
                }

// remove selected donut from the selected ring buffer
// increment the out_ptr mod N

	  	donut=shared_ring->flavor[j][shared_ring->outptr[j]];
	  	shared_ring->outptr[j] = 
			(shared_ring->outptr[j]+1) % NUMSLOTS;
// signal a possible waiting producer that there is now a space in
// the selected ring buffer

		if( v(semid[PROD], j) == -1){
                        perror("v operation failed: ");
                        exit(3);
                }

// unlock the selected out-ptr so it can be used by someone else

	  	if( v(semid[OUTPTR], j) == -1){
                        perror("v operation failed: ");
                        exit(3);
                }
		doz[d[j]][j]=donut;
		d[j] = d[j]+1;

		// printf("donut type %s , \tserial number %d\n",dtype[j],donut);

	   } // end getting one dozen

// print donut column headers for this dozen

	  printf("Jelly\t\tCoconut\t\tPlain\t\tGlazed\n");

// run through 2D doz matrix and print out donut numbers

          for(m=0; m<12; ++m){
		(doz[m][0] == 0)?printf("\t\t"):printf("%d\t\t",doz[m][0]);
		(doz[m][1] == 0)?printf("\t\t"):printf("%d\t\t",doz[m][1]);
		(doz[m][2] == 0)?printf("\t\t"):printf("%d\t\t",doz[m][2]);
		(doz[m][3] == 0)?printf("\t\t\n"):printf("%d\n",doz[m][3]);
		if((doz[m][0] == 0) && (doz[m][1] == 0) && 
                   (doz[m][2] == 0) && (doz[m][3] == 0))break;
	  }


// force context switch with a microsleep

	  usleep(100); // 100 micro-secs is a good value for mercury 2021
	} // end NUMDZ dozen

// print done message via stderr since stdout is being redirected to file

	fprintf(stderr, "          CONSUMER %s DONE\n", argv[1]);
	return 0;
}

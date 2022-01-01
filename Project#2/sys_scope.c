/****************************/
/*      GLOBAL VARIABLES    */
/****************************/

#include "project_header.h"

pthread_mutex_t		prod [NUMFLAVORS];
pthread_mutex_t		cons [NUMFLAVORS];
pthread_cond_t		prod_cond [NUMFLAVORS];
pthread_cond_t		cons_cond [NUMFLAVORS];
pthread_t			thread_id[(NUMCONSUMERS+NUMPRODUCERS)],sig_wait_id;
DonutStore			the_store;


int  main ( int argc, char *argv[] )
{
	int					i, j, k, nsigs;
	struct 				timeval randtime, first_time, last_time;
	struct sigaction	new_act;
	int					arg_array[NUMCONSUMERS + NUMCONSUMERS];
	sigset_t  			all_signals;
	pthread_attr_t 		thread_attr;
	struct sched_param  sched_struct;
	unsigned int		cpu;
	int 				proc_cnt=0;
	int					proc_cntx, cn;
	float				etime;
	ushort				xsub1[3];
	cpu_set_t			mask;
	char				msg[300];
	char				wr_buf[256];
	int sigs[] = { SIGBUS, SIGSEGV, SIGFPE };
	int					first_consumer_thread_index;
	
	gettimeofday ( &randtime, ( struct timezone * ) 0 );
	
	xsub1 [0] = ( ushort )	randtime.tv_usec;
	xsub1 [1] = ( ushort ) ( randtime.tv_usec >> 16 );
	xsub1 [2] = ( ushort ) ( syscall(SYS_gettid) );
	
/********************************************************/
/* CONFIGURE PROCESS AFFINITY SCHEDULING ATTRIBUTES */
/* BEFORE THREAD CREATION IN THE MAIN ROUTINE */
/*******************************************************/

	sched_getaffinity(0, sizeof(cpu_set_t), &mask);
	// GET CPU COUNT FOR THIS PLATFORM
	proc_cnt = get_nprocs();
	
	sprintf( msg, "\nThis run will have\n Producers = %d\n Consumers = %d\n Qdepth = %d\n Cons dozens = %d\n Donut fla = %d\n Thrd scope = %s\n Number CPUs = %d\n", NUMPRODUCERS, NUMCONSUMERS, NUMSLOTS, NUM_DOZ_TO_CONS, NUMFLAVORS, "System", proc_cnt);
	write( STDOUT, msg, strlen(msg) );
	printf("\nPROCESS AFFINITY MASK BEFORE ADJUSTMENT:\n");
	printf(" CPUs : 0 ");
	for(i=1; i < proc_cnt; ++i) printf("%d ", i);
	printf("\n");
	printf("        %d ", (CPU_ISSET(0,&mask))?1:0);
	for(i=1; i < proc_cnt; ++i)printf("%d ",(CPU_ISSET(i, &mask))?1:0);
	printf("\n");

	// INITIALIZE xsub1, USE nrand48 TO SELECT A CPU
	// RANDOMLY, SET MASK FOR ONE CPU FOR ALL THREADS
	//CPU_ZERO(&mask);
	//proc_cntx = (nrand48(xsub1));
	//CPU_SET(proc_cntx%proc_cnt,&mask);
	//sched_setaffinity(0, sizeof(cpu_set_t), &mask);
	
	
	/*printf("\nPROCESS AFFINITY MASK AFTER ADJUSTMENT:\n");
	sched_getaffinity(syscall(SYS_gettid), sizeof(cpu_set_t), &mask);
	printf(" CPUs : 0 ");
	for(i=1; i < proc_cnt; ++i) printf("%d ", i);
	printf("\n");
	printf("        %d ", (CPU_ISSET(0,&mask))?1:0);
	for(i=1; i < proc_cnt; ++i) printf("%d ",(CPU_ISSET(i, &mask))?1:0);*/
	printf("\n\n");
	
/**********************************************************************/
/* INITIAL TIMESTAMP VALUE FOR PERFORMANCE MEASURE                    */
/**********************************************************************/

    gettimeofday (&first_time, (struct timezone *) 0 );
    for ( i = 0; i < NUMCONSUMERS + 1; i++ ) {
		arg_array [i] = i;	/** SET ARRAY OF ARGUMENT VALUES **/
    }

/**********************************************************************/
/* GENERAL PTHREAD MUTEX AND CONDITION INIT AND GLOBAL INIT           */
/**********************************************************************/

	for ( i = 0; i < NUMFLAVORS; i++ ) {
		pthread_mutex_init ( &prod [i], NULL );
		pthread_mutex_init ( &cons [i], NULL );
		pthread_cond_init ( &prod_cond [i],  NULL );
		pthread_cond_init ( &cons_cond [i],  NULL );
		the_store.out_ptr [i]		= 0;
		the_store.in_ptr [i]		= 0;
		the_store.serial [i]		= 1;
		the_store.space_count [i]	= NUMSLOTS;
		the_store.donut_count [i]	= 0;
	}


/**********************************************************************/
/* SETUP FOR MANAGING THE SIGTERM SIGNAL, BLOCK ALL SIGNALS           */
/**********************************************************************/

	sigfillset (&all_signals );
	nsigs = sizeof ( sigs ) / sizeof ( int );
	for ( i = 0; i < nsigs; i++ )
       		sigdelset ( &all_signals, sigs [i] );
	sigprocmask ( SIG_BLOCK, &all_signals, NULL );
	sigfillset (&all_signals );
	
	for( i = 0; i <  nsigs; i++ ) {
      		new_act.sa_handler 	= sig_handler;
      		new_act.sa_mask 	= all_signals;
      		new_act.sa_flags 	= 0;
      		if ( sigaction ( sigs[i], &new_act, NULL ) == -1 ){
         			perror("can't set signals: ");
         			exit(1);
      		}
   	}

	printf ( "just before threads created\n" );

	

/*********************************************************************/
/* CREATE SIGNAL HANDLER THREAD, PRODUCER AND CONSUMERS              */
/*********************************************************************/

	if ( pthread_create (&sig_wait_id, NULL, sig_waiter, NULL) != 0 ){
		printf ( "pthread_create failed " );
        exit ( 3 );
    }

    pthread_attr_init ( &thread_attr );
    pthread_attr_setinheritsched ( &thread_attr, PTHREAD_INHERIT_SCHED );

#ifdef  GLOBAL
    sched_struct.sched_priority = sched_get_priority_max(SCHED_OTHER);
    pthread_attr_setinheritsched ( &thread_attr, PTHREAD_EXPLICIT_SCHED );
    pthread_attr_setschedpolicy ( &thread_attr, SCHED_OTHER );
    pthread_attr_setschedparam ( &thread_attr, &sched_struct );  
    pthread_attr_setscope ( &thread_attr, PTHREAD_SCOPE_SYSTEM );
	
#endif

	for(i=0; i<NUMPRODUCERS;i++){
		if((errno = pthread_create(&thread_id[i],&thread_attr,producer,(void *)&arg_array[i])) != 0){
			perror("pthread_create failed ");
			exit(3);
		}
	}
	first_consumer_thread_index = i;
	for( ; i<(NUMCONSUMERS+NUMPRODUCERS); i++){
		if((errno=pthread_create(&thread_id[i],&thread_attr,consumer,(void *)&arg_array[i-NUMPRODUCERS])) != 0){
			perror("pthread_create consumners failed " );
			exit(3);
		}
	}
	
	for(i=first_consumer_thread_index; i<(NUMCONSUMERS+NUMPRODUCERS); i++){
		pthread_join(thread_id[i],NULL);
		write(STDOUT,"*",1);
	}
	
	//for(i=first_consumer_thread_index; i<(NUMCONSUMERS+NUMPRODUCERS); i++){
		//printf("Pth%d #%d ",i, );
	//}
	
/*****************************************************************/
/* GET FINAL TIMESTAMP, CALCULATE ELAPSED SEC AND USEC           */
/*****************************************************************/

	gettimeofday(&last_time, (struct timezone *)0);
	if((i=last_time.tv_sec - first_time.tv_sec) == 0)
		j=last_time.tv_usec - first_time.tv_usec;
	else{
		if(last_time.tv_usec - first_time.tv_usec < 0){
			i--;
			j = 1000000 + (last_time.tv_usec - first_time.tv_usec);
		}else{ j=last_time.tv_usec - first_time.tv_usec;}
	}
		
	printf("\n\nElapsed consumer time is %d sec and %d usec, or %f sec\n", i, j, (etime =i + (float)j/1000000));
	if((cn = open("./sys_times", O_WRONLY|O_CREAT|O_APPEND, 0644)) == -1){
		perror("can not open sys time file ");
		exit(1);
	}
	
	sprintf(msg, "%f\n", etime);
	write(cn, msg, strlen(msg));
	printf("\nALL CONSUMERS FINISHED, NORMAL PROCESSS EXIT\n\n");
	//exit(0);
	
}
/*********************************************/
/* INITIAL PART OF PRODUCER.....             */
/*********************************************/

void	*producer ( void *arg )
{
	int				i, j, k;
	struct timeval 	randtime;
	float			etime;
	int 			proc_cntx, cn;
	char			number[2] = {'\060','\n'};
	char 			file_name[10] = "prod";
	char 			thread_number[5];
	ushort			xsub1[3];
	cpu_set_t 		mask;
	int 			proc_cnt=0;
	char			msg[300];
	
	gettimeofday ( &randtime, ( struct timezone * ) 0 );
	xsub1 [0] = ( ushort )randtime.tv_usec;
	xsub1 [1] = ( ushort ) ( randtime.tv_usec >> 16 );
	xsub1 [2] = ( ushort ) ( syscall(SYS_gettid) );
	
/********************************************************/
/*  SET AFFINITY */
/********************************************************/
	
	sched_getaffinity(0, sizeof(cpu_set_t), &mask);
	proc_cnt = get_nprocs();
	CPU_ZERO(&mask);
	proc_cntx = (nrand48(xsub1));
	CPU_SET(proc_cntx%proc_cnt,&mask);
	sched_setaffinity(0, sizeof(cpu_set_t), &mask);
	
/********************************************************/

#ifdef	DEBUG
	itoa(pthread_self(),thread_number);
	
	strcat(file_name, thread_number);
	if((cn = open(file_name, O_WRONLY | O_CREAT, 0666)) == -1){
		perror("filed to open producer log");
	}
#endif


	while ( 1 ) {
		for (i=0; 1<60; ++i){
			j = nrand48 ( xsub1 ) & 3;
#ifdef DEBUG
			number[0] = j+060;
			write(cn,number,2);
#endif
			pthread_mutex_lock ( &prod [j] );
			while ( the_store.space_count [j] == 0 ) {
                pthread_cond_wait ( &prod_cond [j], &prod [j] );
			}
			the_store.space_count[j]--;
			the_store.donut_ring_buffers[j][the_store.in_ptr[j]].serial_number = the_store.serial[j];
			the_store.in_ptr[j] = (the_store.in_ptr[j]+1)% NUMSLOTS;
			the_store.serial[j]++;
		
			pthread_mutex_unlock ( &prod [j] );
			pthread_mutex_lock( &cons[j] );
			the_store.donut_count[j]++;
			pthread_mutex_unlock( &cons[j] );
		
			pthread_cond_signal(&cons_cond[j]);
		}
//	pthread_yield();
	usleep(100);
	}
	return NULL;
}


/*********************************************/
/* ON YOUR OWN FOR THE CONSUMER.........     */
/*********************************************/


void    *consumer ( void *arg )
{
	int     		i, j, k, id, len;
	unsigned short 	xsub1 [3];
	struct timeval 	randtime;
	id = *( int * ) arg;
	donut_t	extracted_donut;
	char			msg[100], lbuf[5];
	FILE*	cn;
	unsigned cpu;
	char			number[2] = {'\060','\n'};
	char 			file_name[10] = "cons";
	char 			thread_number[5];
	unsigned int 	cpusetsize;
	cpu_set_t 		mask;
	int 			doz[12][4];
	int				d[4], m , n;
	time_t 			t;
	struct 			tm*tp;
	float			ms;
	int 			ims;
	int 			proc_cnt=0;
	int				proc_cntx;
	
	gettimeofday ( &randtime, ( struct timezone * ) 0 );
	xsub1 [0] = ( ushort )randtime.tv_usec;
	xsub1 [1] = ( ushort ) ( randtime.tv_usec >> 16 );
	xsub1 [2] = ( ushort ) ( syscall(SYS_gettid) );
	
#ifdef DEBUG
	itoa(pthread_self(), thread_number);
	
	strcat(file_name, thread_number);
	
	if((cn = fopen(file_name, "w+")) == -1){
		perror("failed to open consumer log");
	}
#endif	

/********************************************************/
/*  SET AFFINITY */
/********************************************************/
		
	sched_getaffinity(0, sizeof(cpu_set_t), &mask);
	proc_cnt = get_nprocs();
	CPU_ZERO(&mask);
	proc_cntx = (nrand48(xsub1));
	CPU_SET(proc_cntx%proc_cnt,&mask);
	sched_setaffinity(0, sizeof(cpu_set_t), &mask);
	
/********************************************************/

	id = *(int*)arg;
	if((id%10) == 9){
		sprintf(file_name, "cons%d", id);
	
	if((cn = fopen(file_name, "w+")) == NULL){
		perror("failed to open output file");
	}
	}
	sprintf(lbuf," %d ", id);
	
	for (i=0; i < NUM_DOZ_TO_CONS; i++){
//zero dozen array and flavor indecies
		for(m=0; m<12; ++m)
			for(n=0; n<4; ++n) doz[m][n] = 0;
		for(n=0; n<4; ++n) d[n] = 0;
//get info for dozen header, including PID, time, doz number
	t = time(NULL);
	tp = localtime(&t);
	gettimeofday(&randtime,NULL);
//get fractional second value as float
	ms = (float)randtime.tv_usec/1000000;
//convert to 3 digit int for printf below
	ims = (int)(ms*1000);
// print dozen header to stdout
	if ((i<10 || i>20000) && ((id%10) == 9)){
		fprintf(cn,"consumer PID %d   time: %d:%d:%d.%d   dozen number: %d   \n\n",
		syscall(SYS_gettid), tp->tm_hour, tp->tm_min, tp->tm_sec, ims, i+1);
	}
	for( m=0; m<12; m++ ) {
		j = nrand48( xsub1 ) & 3;
		
#ifdef DEBUG
		number[0] = j+060;
		write(cn,number,2);
		
#endif

		pthread_mutex_lock ( &cons [j] );
		while ( the_store.donut_count [j] == 0 ) {
            pthread_cond_wait ( &cons_cond [j], &cons [j] );
		}
		the_store.donut_count[j]--;
		extracted_donut = the_store.donut_ring_buffers[j][the_store.out_ptr[j]];
		the_store.out_ptr[j] = (the_store.out_ptr[j]+1)% NUMSLOTS;
		pthread_mutex_unlock ( &cons [j] );
		pthread_mutex_lock ( &prod [j] );
		the_store.space_count[j]++;
		pthread_mutex_unlock ( &prod [j] );
		
		pthread_cond_signal(&prod_cond[j]);
		
		doz[d[j]][j] = extracted_donut.serial_number;
		d[j] = d[j]+1;
	
	} // end getting one dozen
	
//print donut column headers for this dozen
	if((i<10 || i>20000) && ((id%10) == 9 )){
		fprintf(cn,"Jelly\t\tCoconut\t\tPlain\t\tGlazed\n");

//run through 2D doz matrix and print out the donut numbers
		for(m=0;m<12; ++m){
			(doz[m][0] == 0)?fprintf(cn,"\t\t"):fprintf(cn,"%d\t\t",doz[m][0]);
			(doz[m][1] == 0)?fprintf(cn,"\t\t"):fprintf(cn,"%d\t\t",doz[m][1]);
			(doz[m][2] == 0)?fprintf(cn,"\t\t"):fprintf(cn,"%d\t\t",doz[m][2]);
			(doz[m][3] == 0)?fprintf(cn,"\t\t\n"):fprintf(cn,"%d\n",doz[m][3]);
			if((doz[m][0] == 0) && (doz[m][1] == 0) && (doz[m][2] == 0) && (doz[m][3] == 0))break;
		}
	}
//force context switch with a pthread_yield
	usleep(100);
//	pthread_yield();
//	sched_yield();
	}
	write(1,lbuf,4);
    return NULL;
}

/***********************************************************/
/* PTHREAD ASYNCH SIGNAL HANDLER ROUTINE...                */
/***********************************************************/

void    *sig_waiter ( void *arg )
{
	sigset_t	sigterm_signal;
	int			signo;

	sigemptyset ( &sigterm_signal );
	sigaddset ( &sigterm_signal, SIGTERM );
	sigaddset ( &sigterm_signal, SIGINT );


	if (sigwait ( &sigterm_signal, & signo)  != 0 ) {
		printf ( "\n  sigwait ( ) failed, exiting \n");
		exit(2);
	}
	printf ( "process going down on SIGNAL (number %d)\n\n", signo );
	exit ( 1 );
	return NULL;
}


/**********************************************************/
/* PTHREAD SYNCH SIGNAL HANDLER ROUTINE...                */
/**********************************************************/

void	sig_handler ( int sig ) {
	pthread_t	signaled_thread_id;
	int			i, thread_index;

	signaled_thread_id = pthread_self ( );
	for ( i = 0; i < (NUMCONSUMERS + 1 ); i++) {
		if ( signaled_thread_id == thread_id [i] )  {
				thread_index = i;
				break;
		}
	}
	printf ( "\nThread %d took signal # %d, PROCESS HALT\n", thread_index, sig );
	exit ( 1 ); 
}

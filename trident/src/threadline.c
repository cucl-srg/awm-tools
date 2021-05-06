/******************************************************************************
*                                                                             *
*   Copyright 2006 Queen Mary University of London                            *
*   Copyright 2005 University of Cambridge Computer Laboratory.               *
*                                                                             *
*                                                                             *
*   This file is part of Trident.                                             *
*                                                                             *
*   Trident is free software; you can redistribute it and/or modify           *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   Trident is distributed in the hope that it will be useful,                 *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with Trident; if not, write to the Free Software                    *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*                                                                             *
******************************************************************************/



#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/time.h> /* struct timeval */
#include <getopt.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>


#include "trc.h"
#include "trident.h"
#include "trident_util.h"
#include "surge_clt.h"

/*********************************************************************
 * Global variables 
 */


#define STATUS_RUNNING  1
#define STATUS_RETURNED 2
#define STATUS_READY    3


#define MAX_THREADS 256

char        *prog_name;
int G_sched = SCHED_OTHER;

extern char *optarg;
extern int optind, opterr, optopt;

static const struct option long_options[] =
{
    { "interpage",  1, 0, 1001 },
    { "objperpage", 1, 0, 1002 },
    { "interobj",   1, 0, 1003 },
    { "objsize",    1, 0, 1004 },
    /* END OF ARRAY MARKER */
    { 0,            0, 0,    0 }
};

typedef struct runRecord {
    alldistn mydistn;
    stats current_stats;
    struct timeval lastUpdatedRR;
    struct timeval prevUpdatedRR;
    int lock1;
    int lock2;
    int status;
} runRecord;



typedef  struct {
    int id;
    int simultaneous_flows;
    char *dhost;
    int num_ports;
    int base_port;
    runRecord *runRecord;
    struct timeval run_time;
}  surge_arg;


static distn interpage;  /* distn. function for interpage time    */
static distn objperpage; /* distn. function for objects per page  */
static distn interobj;   /* distn. function for inter-object time */
static distn objsize;    /* distn. function for bytes per object  */


enum {
    Distn_Function_Constant,
    Distn_Function_Exponent,
    Distn_Function_Pareto,
    Distn_Function_nTypes
};

static const char *Distn_Function_Type[Distn_Function_nTypes] = {
    "constant",
    "exponent",
    "pareto"
};


typedef struct shmemt {

    runRecord *rR;
    
} shmemt;


static void th_getlock1(runRecord *ptr);
#if 0
static int th_getlock1retry(runRecord *ptr,int retry);
#endif /* 0 */
static void th_getlock2(runRecord *ptr);
static void th_releaselock2(runRecord *ptr);
static void th_releaselock1(runRecord *ptr);



void wrapper_surge_clt(surge_arg *arg) {
    struct timeval current_time;

    fprintf(stderr,":%p %d %s %d %d %p %d.%d\n",
	    arg,
	    arg->simultaneous_flows,
	    arg->dhost,
	    arg->num_ports,
	    arg->base_port,
	    arg->runRecord,
	    arg->run_time.tv_sec,
	    arg->run_time.tv_usec, __FUNCTION__);

    (void)surge_client(arg->simultaneous_flows,
		 arg->dhost,
		 arg->num_ports,
		 arg->base_port,
		 &arg->runRecord->current_stats,
		 &arg->run_time,
		 &arg->runRecord->mydistn);

    gettimeofday(&current_time,NULL);

    arg->runRecord->lastUpdatedRR=current_time;
    
    printf("thread term\n");

    return;

}


int
parse_distn(distn *distribution,int optind,char **argv, int argc)
{
    int rv=0,rc;
    int theType;

    optind--;


    for ( theType = 0; theType < Distn_Function_nTypes; theType++ ) 
    {
        if ( !strncmp( argv[optind] , Distn_Function_Type[theType],255 ) ) 
	    break;
    }


    switch(theType) 
    {
	case Distn_Function_Constant:
	{
	    int argindex=optind;

		if((argc - optind) < 2) {
			fprintf(stderr,"%s: constant requries one argument\n",__FUNCTION__);
	    	rv = -1;
			goto abort;
		}

	    /* one additional argument for a constant generator, the
	     * constant itself (or `mean') */
	    distribution->function = constant;

	    argindex++;

	    rc = sscanf(argv[argindex],"%lg",&distribution->mean);
	    if(rc != 1) 
	    {
		fprintf(stderr, "%s: bad constant %s \n",
			__FUNCTION__, argv[argindex]);
	    	rv = -1;
		goto abort;
	    }

	}
	break;
	case Distn_Function_Exponent:
	{
	    int argindex=optind;
	    /* one additional argument for a exponential generator the
	       mean */
	    distribution->function=expon;

		if((argc - optind) < 2) {
			fprintf(stderr, "%s: exponent requries one argument (mean)\n",
				__FUNCTION__);
	    	rv = -1;
			goto abort;
		}



	    argindex++;
	
	    rc = sscanf(argv[argindex],"%lg",&distribution->mean);
	    if(rc != 1) 
	    {
		fprintf(stderr,
			 "%s: bad constant %s \n",__FUNCTION__,argv[argindex]);
	    	rv = -1;
		goto abort;
	    }


	}
	break;
	case Distn_Function_Pareto:
	{
	    int argindex=optind;
	    /* two additional arguments for a pareto generator
	       the mean and the shape */
	    distribution->function=paretoon;

		if((argc - optind) < 3) {
			fprintf(stderr, "%s: pareto requries two arguments (mean and shape)\n",
				__FUNCTION__);
	    	rv = -1;
			goto abort;
		}

	    argindex++;
	    rc = sscanf(argv[argindex],"%lg",&distribution->mean);
	    if(rc != 1) 
	    {
		fprintf(stderr,
			 "%s: bad mean %s \n",__FUNCTION__,argv[argindex]);
	    	rv = -1;
		goto abort;
	    }
	    argindex++;

	    rc = sscanf(argv[argindex],"%lg",&distribution->shape);
	    if(rc != 1) {
		fprintf(stderr, "%s: bad shape %s \n",
			__FUNCTION__, argv[argindex]);
	    	rv = -1;
		goto abort;
	    }
	}
	break;
	default:
	  fprintf(stderr,"%s: unknown distribution type %s\n",__FUNCTION__,
		    argv[optind]);
	    rv = -1;
    }

 abort:
    return rv;
}

void
usage(char *name)
{
    fprintf(stderr,
	    "Syntax error: usage is one of\n"
	    "[svr] %s -s \n"
	    "[clt] %s -c <server> [-n <conns>] [-b <bytes>]\n"
	    "[rxr] %s -r\n"
	    "[txr] %s -t <receiver> [-n <conns>] [-b <bytes>]\n"
	    "[surge_clt] %s -C <server> [-n <conns>]\n"
		"        [--<objsize|interobj|objperpage|interpage>\n"
		"         <constant <value>|exponent <mean>|pareto <mean> <shape>>]\n"
	    "commands valid for all generators\n"
	    "[-D <scaling> (of timers)]\n"
	    "[-T <runtime>]\n"
	    "[-B <port base>]\n"
	    "[-N <number of ports>]\n"
	    "[-R <random number seed>]\n",
	    name, name, name, name, name);
    exit(1);
}

int 
main(int argc, char **argv)
{
    int rc, c, n = 1, i, bytes = 512*1025;
    char dhost[256];
    struct timeval run_time = {INT_MAX,0},start_time;
    alldistn alldistns;
    shmemt *ptr;
    surge_arg arg[MAX_THREADS];
    pthread_t thread[MAX_THREADS];
    pthread_attr_t attr[MAX_THREADS];
/* 	struct sched_param schedParam; */
/* 	int currentSchedPolicy; */


    ptr=calloc(1,sizeof(shmemt));

    if(geteuid() == (uid_t)0) {
	fprintf(stderr,"Using Round-robin scheduler\n");
	G_sched = SCHED_RR;
    }

    ptr->rR=calloc(MAX_THREADS,sizeof(runRecord));
    if(!ptr->rR) {
      fprintf(stderr, "%s: unable to malloc runRecords\n",__FUNCTION__);
	goto abort; /* its ghastly but I'm trying to get a single
		       entry/exit point */
    }

    for(i=0;i<MAX_THREADS;i++) {
	ptr->rR[i].lock1=ptr->rR[i].lock2=0;
	ptr->rR[i].status=STATUS_READY;
    }

    interpage.mean      = 0;
    interpage.function  = constant;
    objperpage.mean     = 1;
    objperpage.function = constant;
    interobj.mean       = 0;
    interobj.function   = constant;
    objsize.mean        = 1e6;
    objsize.function    = constant;

    alldistns.interpage=interpage;
    alldistns.objperpage=objperpage;
    alldistns.interobj=interobj;
    alldistns.objsize=objsize;


    while((c = getopt_long(argc, argv, "S:vR:B:N:D:T:C:rt:sc:n:b:", 
			   long_options, NULL)) != -1) 
    {
	switch(c) 
	{
	case 'v':
	    verbose++;
	    break;
	case 1001:
	    rc = parse_distn(&interpage, optind, argv, argc);
	    if(rc<0) 
	    {
		usage(argv[0]);
	    }
	    break;
	case 1002:
	    rc = parse_distn(&objperpage, optind, argv, argc);
	    if(rc<0) 
	    {
		usage(argv[0]);
	    }
	    break;
	case 1003:
	    rc = parse_distn(&interobj, optind, argv, argc);
	    if(rc<0) 
	    {
		usage(argv[0]);
	    }
	    break;
	case 1004:
	    rc = parse_distn(&objsize, optind, argv, argc);
	    if(rc<0) 
	    {
		usage(argv[0]);
	    }
	    break;
	case 'R':
	{
	    unsigned short seed_16v[3];
	    unsigned long  full_seed;
	    int            rc;
	    
	    rc = sscanf(optarg,"%ld",&full_seed);
            
	    if(rc != 1) 
	    {
		usage(argv[0]);
	    }
                        
	    seed_16v[0] = (short)(0xffffl & full_seed);
	    seed_16v[1] = (short)((~0xffffl & full_seed) >> 16);
	    seed_16v[2] = (short)((~0xffl & full_seed) >> 8); /* XXX */
	    (void)seed48(seed_16v);
	}
	break;
	case 'r':          /* receiver */
	    if(mode == unset)
	    {
		mode = rx;
	    }
	    else
	    {
		usage(argv[0]);
	    }
	    break;
      
	case 't':          /* transmitter */
	    if(mode == unset)
	    {
		mode = tx;
	    }
	    else
	    {
		usage(argv[0]);
	    }

	    strncpy(dhost, optarg, 255);
	    break;

	case 's':          /* server */
	    if(mode == unset)
	    {
		mode = svr;
	    }
	    else
	    {
		usage(argv[0]);
	    }
	    break;

	case 'c':          /* client */
	    if(mode == unset)
	    {
		mode = cts_clt;
	    }
	    else
	    {
		usage(argv[0]);
	    }

	    strncpy(dhost, optarg, 255);
	    break;

	case 'S':          /* surge client */
	    if(mode == unset)
	    {
			
		mode = surge_clt;
	    }
	    else
	    {
		usage(argv[0]);
	    }
	    strncpy(dhost, optarg, 255);
	    break;

	case 'n':          /* number of concurrent connections */
	    n = getint(optarg);
	    break;

	case 'b':          /* number of bytes in each connection */
	    bytes = getint(optarg);
	    break;

	case 'B':
	    base_port = getint(optarg);
	    break;

	case 'N':
	    num_ports = getint(optarg);
	    break;

	case 'D':          /* scaling */
	    scalar = getdouble(optarg);
	    fprintf(stderr, "using a scalar of %g\n", scalar);
	    break;

	case 'T':          /* run time */
	{
	    double runtime_sec;
	    runtime_sec = getdouble(optarg);
	    run_time.tv_sec = (int) runtime_sec;
	    run_time.tv_usec = (int)( (runtime_sec 
				       - (double)run_time.tv_sec) 
				      / (double)1e6);
	    break;
	}
	default:
	    usage(argv[0]);
	}
    }

    /* SIGPIPE caused by RST connection; ignore and deal with errno
     * from initial write */
    signal(SIGPIPE, SIG_IGN); 
    signal(SIGINT,  shut_sockets);



/*     if(run_time >= 0)  */
/*     { */
/* 	struct itimerval run_time_timerval; */
/* 	int rc; */
	
/* 	run_time = run_time * scalar; */
	
/* 	run_time_timerval.it_interval.tv_sec = 0; */
/* 	run_time_timerval.it_interval.tv_usec = 0; */
	
/* 	run_time_timerval.it_value.tv_sec = (int) run_time; */
/* 	run_time_timerval.it_value.tv_usec =  */
/* 	    (int) ((run_time - (double)run_time_timerval.it_value.tv_sec) * 1e6 ); */

/* 	if(verbose) { */
/* 		fprintf(stderr,"run time %d.%06d\n", */
/* 			(int)run_time_timerval.it_value.tv_sec, */
/* 			(int)run_time_timerval.it_value.tv_usec); */
/* 	} */
	
	
/* 	rc = setitimer(ITIMER_REAL, &run_time_timerval,NULL); */
/* 	if(rc != 0)  */
/* 	{ */
/* 	    perror("setitimer"); */
/* 	    exit(-1); */
/* 	} */
/*     } */

    gettimeofday(&start_time,NULL);
    switch(mode)    
    {
    case unset:
	usage(argv[0]);
    case rx:
	simple_rx(num_ports, base_port);
	break;
    case tx:
	simple_tx(n, bytes, dhost, num_ports, base_port);
	break;
    case svr:
	simple_server(num_ports, base_port);
	break;
    case simple_clt:
	simple_client(n, bytes, dhost, num_ports, base_port);
	break;
    case cts_clt:
	continuous_client(n, bytes, dhost, num_ports, base_port);
	break;
    case surge_clt: 
    

/* XXX put a check i !> MAX_THREADS */


	for(i=0;i<n;i++) {

	    arg[i].id=i;
	    arg[i].simultaneous_flows=5;
	    arg[i].dhost=dhost;
	    arg[i].num_ports=num_ports;
	    arg[i].base_port=base_port;
	    arg[i].runRecord=&ptr->rR[i];
	    ptr->rR[i].mydistn = alldistns;
	    arg[i].run_time=run_time;
/*	    arg[i].alldistns=&alldistns;*/
	  
	    fprintf(stderr,":%d %p %d %s %d %d %p %d.%d\n", i,
		    &arg[i],
		    arg[i].id,
		    arg[i].dhost,
		    arg[i].num_ports,
		    arg[i].base_port,
		    arg[i].runRecord,
		    arg[i].run_time.tv_sec,
		    arg[i].run_time.tv_usec , __FUNCTION__);
	  

	    rc=pthread_attr_init(&attr[i]);
	    if(rc){
		perror("pthread_attr_init");
		exit(1);
	    }

	    rc=pthread_create(&thread[i],
			      &attr[i], 
			      &wrapper_surge_clt,
			      (void *) &arg[i]);
	    if(rc){
		perror("pthread_create");
		exit(1);
	    }

/* this doesn't work, must come back here XXX */ 

/* 	    if(G_sched != SCHED_OTHER) { */
/* 		rc=pthread_getschedparam(thread[i],&currentSchedPolicy, */
/* 					 &schedParam); */
/* 		if(rc){ */
/* 		    perror("pthread_getschedparam"); */
/* 		    exit(1); */
/* 		} */
/* 		rc=pthread_setschedparam(thread[i],G_sched,&schedParam); */
/* 		if(rc){ */
/* 		    perror("pthread_setschedparam"); */
/* 		    exit(1); */
/* 		} */
/* 	    } */
	}
			     
	break;
    }

    while(1) {
	sleep(1);

	if(mode == surge_clt ) {
	    struct timeval current_time,running_time;

	    gettimeofday(&current_time,NULL);

/* 	    fprintf(stderr, */
/* 		    "%d.%06d %d.%06d %p %d After %ld bytes %ld conns" */
/* 		    " %ld.%03lds = %.4f Mb/s. \n",  */
/* 		    current_time.tv_sec, */
/* 		    current_time.tv_usec, */
/* 		    ptr->rR[0].lastUpdatedRR.tv_sec,			 */
/* 		    ptr->rR[0].lastUpdatedRR.tv_usec,			 */
/* 		    &ptr->rR[0].current_stats,			 */
/* 		    (long int)ptr->rR[0].current_stats.tbytes, */
/* 		    (unsigned long)ptr->rR[0].current_stats.conns, */
/* 		    (long int)(ptr->rR[0].current_stats.ttime_usec/1e6),  */
/* 		    (long int)(ptr->rR[0].current_stats.ttime_usec/1e6 - */
/* 			       (long int)(ptr->rR[0].current_stats.ttime_usec/1e6) ),  */
/* 		    ((double)(8.0*ptr->rR[0].current_stats.tbytes) / */
/* 		     (double)(ptr->rR[0].current_stats.ttime_usec))/scalar); */
		    
	    for(i=0;i<MAX_THREADS;i++) {

		if(tvless(&ptr->rR[i].prevUpdatedRR,
			    &ptr->rR[i].lastUpdatedRR)) {
		    
		    
		    tvsub(&running_time,&current_time, &start_time);
		    
		    fprintf(stderr,
			    "%p %d After %ld bytes %ld conns"
			    " %ld.%03lds = %.4f Mb/s. %ld.%03lds"
			    " = %.4f Mb/s.\n", 
			    &ptr->rR[i].current_stats,			
			    i,
			    (long int)ptr->rR[i].current_stats.tbytes,
			    (unsigned long)ptr->rR[i].current_stats.conns,
			    (long int)(ptr->rR[i].current_stats.ttime_usec/1e6), 
			    (long int)(ptr->rR[i].current_stats.ttime_usec/1e6 -
				       (long int)(ptr->rR[i].current_stats.ttime_usec/1e6) ), 
			    ((double)(8.0*ptr->rR[i].current_stats.tbytes) /
			     (double)(ptr->rR[i].current_stats.ttime_usec))/scalar,
			    (long int) running_time.tv_sec,
			    (long int) running_time.tv_usec,
			    ((double)(8.0*ptr->rR[i].current_stats.tbytes) /
			     (double)(running_time.tv_sec*1e6 + running_time.tv_usec)
				)/scalar);


		    ptr->rR[i].prevUpdatedRR=
			ptr->rR[i].lastUpdatedRR;

		    
		}
	    }
	}
    }
    return 0;
abort:
    return -1;

}















static void
th_releaselock1(runRecord *ptr) {
    ptr->lock1--;
    ptr->lock2--;
}


static void
th_getlock1(runRecord *ptr) {
    struct timeval sleepval,sleepval_master={0,1};

top:
    sleepval=sleepval_master;
    select(0,NULL,NULL,NULL,&sleepval);

    if(ptr->lock1) {
        goto top;
    } else {
        ptr->lock1++;
        if(ptr->lock2) {
            ptr->lock1--;
            goto top;
        } else {
            ptr->lock2++;
        }
    }
}


static void
th_releaselock2(runRecord *ptr) {

    ptr->lock2--;
    ptr->lock1--;
}

static void
th_getlock2(runRecord *ptr) {
    struct timeval sleepval,sleepval_master={0,1};
top:
    sleepval=sleepval_master;
    select(0,NULL,NULL,NULL,&sleepval);

    if(ptr->lock2) {
        goto top;
    } else {
        ptr->lock2++;
        if(ptr->lock1) {
            ptr->lock2--;
            goto top;
        } else {
            ptr->lock1++;
        }
    }
}


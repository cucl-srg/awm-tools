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


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <unistd.h>


#include "trc.h"
#include "trident.h"
#include "trident_util.h"
#include "surge_clt.h"

static int     G_os;

#define interpage_distn(a)  (a)->interpage.function(&(a)->interpage)
#define objperpage_distn(a)  (((G_os = (a)->objperpage.function(&(a)->objperpage)) < 1) ? 1 : G_os) 
#define interobj_distn(a)   (a)->interobj.function(&(a)->interobj)
/* I couldn't remmeber how to code this without an external variable 
   silly really */
#define objsize_distn(a)    (((G_os = (a)->objsize.function(&(a)->objsize)) < 1) ? 1 : G_os)

#define interpage_mean(a)  a->interpage.mean
#define objperpage_mean(a) a->objperpage.mean
#define interobj_mean(a)   a->interobj.mean
#define objsize_mean(a)    a->objsize.mean

#define PRINT_SET(a,c)
#if 0
#define PRINT_SET(a,c) {\
int i;\
fprintf(stderr,"%s ",c);\
for(i=0;i<maxfd;i++) {if(FD_ISSET(i,&a)) {fprintf(stderr,"%d,",i);}}\
fprintf(stderr,"\n");}
#endif /* 0 */

/*********************************************************************
 * surge client


 * up to date statistics are returned in stats structure 
 */
int 
surge_client(int n, char *host, int num_ports, int base_rx_port, stats *stats,
	     struct timeval *run_time,alldistn *alldistns)
{
    state_t state[__FD_SETSIZE];
    int i;
    fd_set fds_new, fds_active, fds_finished, ids_sleeping, ids_want_to_run;
    unsigned long ipaddr;    
    double time_us;

    struct timeval next_start_time = {0,0};
    struct timeval current_time;
    struct timeval end_time;

/*    verbose++;*/
    ipaddr = host2ipaddr(host);

    FD_ZERO(&fds_new);
    FD_ZERO(&fds_active);
    FD_ZERO(&fds_finished);
    FD_ZERO(&ids_sleeping);
    FD_ZERO(&ids_want_to_run);

    /* calculate end time */
    if(run_time->tv_sec < INT_MAX) {
	gettimeofday(&current_time,NULL);
	tvadd(&end_time,run_time,&current_time);

    }else
	end_time = *run_time;

    /* there is a bit of repitition of code here, the first for() is
       initialisation, the while() is the action loop */

    /* create all the TCP connections */
    for(i=0; i<n; i++)
    {
	int fd;

	fd = create_tx_tcp(state, ipaddr, base_rx_port+(i%num_ports));

	if(fd < 0)
	{
	  fprintf(stderr, " %s (%d): Unable to open connection!\n",__FUNCTION__,
		    __LINE__);
	    return(-1);
	}

	if(fd > maxfd) 
	    maxfd = fd;


	state[fd].client_id=fd;

	state[fd].object_count=objperpage_distn(alldistns);

	state[fd].tx_target = objsize_distn(alldistns);
	state[fd].tx_sent   = 0;
	state[fd].tx_pkts   = 0;
	state[fd].rx_rcvd   = 0;

	time_us = 0;
/*	if(time_us<=0) {
// 	    time_us += 0.001;
	} 
	fprintf(stderr,__FUNCTION__": time_us %g\n",time_us); */
	time_us *= scalar;
	next_start_time.tv_sec = (int) time_us;
	next_start_time.tv_usec = (int) 
	    ((time_us - (double)next_start_time.tv_sec) 
	     * 1e6 );
	gettimeofday(&state[fd].rx_stop, (struct timezone *)0);
	tvadd(&state[state[fd].client_id].next_start_time,
	      &state[fd].rx_stop,
	      &next_start_time);

/* 	FD_CLR(fd, &fds_active); */
/* 	FD_SET(state[fd].client_id, &ids_sleeping); 
 	close(fd); 
 	state[fd].open = 0;  */

	FD_SET(fd, &fds_new);



#if 0
	/* this is code for offestting the starts of each connection but its all a bit pox. */

	if (drand48() > (interpage_mean(alldistns) + interobj_mean(alldistns)) /
	    (interpage_mean(alldistns) + interobj_mean(alldistns) + objperpage_mean(alldistns) +
	     (objsize_mean(alldistns) / 1.25e6))) {
	    /* start transmitting */
	    FD_SET(fd, &fds_new);

	}else{
	    double test_time=0;
	    if((interpage_mean(alldistns) + interobj_mean(alldistns)) > 0 ) {
		test_time=(interpage_mean(alldistns) / 
			   (interpage_mean(alldistns) + interobj_mean(alldistns)));
	    }
	    /* sleep */
	    if (drand48() < test_time) {
		/* sleep for an interpage */

		time_us = interpage_distn(alldistns);
		time_us *= drand48();
	    }else{
		/* sleep for an interobj */
		
		time_us = interobj_distn(alldistns);
		time_us *= drand48();
	
	    }


/* 	    if(verbose) */
/* 		       (int)next_start_time.tv_sec, */
/* 		       (int)next_start_time.tv_usec, */
/* 		       state[fd].object_count); */


	}
#endif /* 0 */
    }


    /* start sending */
    while(tveqless(&current_time,&end_time))
    {
	int s, fd, client_id, sink_data_rc;
	struct timeval tdiff;


	if( send_request(state, &fds_new, &fds_active) < 0 ) {
	  fprintf(stderr,"%s: send_request returned error\n",__FUNCTION__);
	    return(-1);
	}



    if(verbose) {
    fprintf(stdout,"%d.%06d new %d active %d finished %d sleeping %d want %d (%d)\n",  
	    (int)current_time.tv_sec,(int)current_time.tv_usec, 
	    FD_POP(maxfd,&fds_new), 
	    FD_POP(maxfd,&fds_active), 
	    FD_POP(maxfd,&fds_finished), 
	    FD_POP(maxfd,&ids_sleeping), 
	    FD_POP(maxfd,&ids_want_to_run), 
	    (FD_POP(maxfd,&fds_new)+ 
	     FD_POP(maxfd,&fds_active)+ 
	     FD_POP(maxfd,&fds_finished)+ 
	     FD_POP(maxfd,&ids_sleeping)+ 
	     FD_POP(maxfd,&ids_want_to_run))); 
	}

	sink_data_rc = sink_data(state, &fds_active, &fds_finished);



    if(verbose)
    fprintf(stdout,"%d.%06d new %d active %d finished %d sleeping %d want %d (%d)\n",  
	    (int)current_time.tv_sec,(int)current_time.tv_usec, 
	    FD_POP(maxfd,&fds_new), 
	    FD_POP(maxfd,&fds_active), 
	    FD_POP(maxfd,&fds_finished), 
	    FD_POP(maxfd,&ids_sleeping), 
	    FD_POP(maxfd,&ids_want_to_run), 
	    (FD_POP(maxfd,&fds_new)+ 
	     FD_POP(maxfd,&fds_active)+ 
	     FD_POP(maxfd,&fds_finished)+ 
	     FD_POP(maxfd,&ids_sleeping)+ 
	     FD_POP(maxfd,&ids_want_to_run))); 


	gettimeofday(&current_time,NULL);
	/* close those that have finished */
	for(s=0; 
	    (sink_data_rc>0) && (fd = FD_FFSandC(s, maxfd, &fds_finished));
	    s = fd+1)
	{
	    double mbs;

	    tvsub(&tdiff, &state[fd].rx_stop, &state[fd].rx_start);
	  
	    mbs = (double)(8.0*state[fd].rx_rcvd) /
		(double)(tdiff.tv_sec*1.e6 + tdiff.tv_usec);

	    stats->conns++;
	    stats->tbytes += (state[fd].rx_rcvd);
	    stats->ttime_usec += (double)(tdiff.tv_sec*1.e6 + tdiff.tv_usec);
	    

	    if(1) {
		fprintf(stderr,
			"%d.%06d Finished with %d after %d bytes. "
			"%ld.%03lds = %.4f Mb/s.\n", 
			(int)current_time.tv_sec,(int)current_time.tv_usec,
			fd, state[fd].rx_rcvd, 
			(long int)tdiff.tv_sec, (long int)tdiff.tv_usec/1000, 
			mbs/scalar);


		fprintf(stderr,
			"%p After %ld bytes %ld conns %ld.%06lds = %.4f Mb/s.\n",
			stats,
			(long int)stats->tbytes,
			(unsigned long)stats->conns,
			(long int)(stats->ttime_usec/1e6), 
			(long int)((stats->ttime_usec/1e6 -
				   (long int)(stats->ttime_usec/1e6) )*1e6), 
			((double)(8.0*stats->tbytes) /
			 (double)(stats->ttime_usec))/scalar
		    );


	    }
	    close(fd);
	    state[fd].open = 0;

	    /* control loop for time keeping:
	     *
	     * this should embody the idea that if the object count
	     * is >1 insert an interobj sleep; if the object count is
	     * <1 recalc the object count and this sleep is an
	     * interpage sleep.
	     */

	    state[fd].object_count--;

	    if(state[fd].object_count > 0) {
		
		time_us = interobj_distn(alldistns);
	    }else{
		time_us = interpage_distn(alldistns);
		state[fd].object_count=objperpage_distn(alldistns);
	    }


	    time_us *= scalar;
	    next_start_time.tv_sec = (int) time_us;
	    next_start_time.tv_usec = (int) 
		((time_us - (double)next_start_time.tv_sec) 
		 * 1e6 );
	    tvadd(&state[state[fd].client_id].next_start_time,
		  &state[fd].rx_stop,
		  &next_start_time);

/* 	    if(verbose) */
/* 		       (int)next_start_time.tv_sec, */
/* 		       (int)next_start_time.tv_usec, */
/* 		       state[fd].object_count); */
	    FD_CLR(fd, &fds_finished);
	    FD_SET(state[fd].client_id, &ids_sleeping);
	}



    if(verbose)
    fprintf(stdout,"%d.%06d new %d active %d finished %d sleeping %d want %d (%d)\n",  
	    (int)current_time.tv_sec,(int)current_time.tv_usec, 
	    FD_POP(maxfd,&fds_new), 
	    FD_POP(maxfd,&fds_active), 
	    FD_POP(maxfd,&fds_finished), 
	    FD_POP(maxfd,&ids_sleeping), 
	    FD_POP(maxfd,&ids_want_to_run), 
	    (FD_POP(maxfd,&fds_new)+ 
	     FD_POP(maxfd,&fds_active)+ 
	     FD_POP(maxfd,&fds_finished)+ 
	     FD_POP(maxfd,&ids_sleeping)+ 
	     FD_POP(maxfd,&ids_want_to_run))); 


	for(s=0;(client_id = FD_FFS(s, maxfd, &ids_sleeping));
	    s = client_id+1)
	{
	

	    if(tveqless(&state[client_id].next_start_time,&current_time)) {
		if(state[client_id].object_count > 0) {
		    FD_CLR(client_id, &ids_sleeping);
		    FD_SET(client_id, &ids_want_to_run);
		}else{

		    /* this exception handles the empty page in which
		       case if the time expires we just wait another
		       interpage and calculate another value for the
		       objperpage */


		    time_us = interpage_distn(alldistns);
		    time_us *= scalar;



		    state[client_id].object_count=objperpage_distn(alldistns);



		    next_start_time.tv_sec = (int) time_us;



		    next_start_time.tv_usec = (int) 
			((time_us - (double)next_start_time.tv_sec) 
			 * 1e6 );


		    tvadd(&state[state[client_id].client_id].next_start_time,
			  &state[client_id].rx_stop,
			  &next_start_time);

		}
	    }
	}



    if(verbose)
    fprintf(stdout,
            "%d.%06d new %d active %d finished %d sleeping %d want %d (%d)\n",  
	    (int)current_time.tv_sec,(int)current_time.tv_usec, 
	    FD_POP(maxfd,&fds_new), 
	    FD_POP(maxfd,&fds_active), 
	    FD_POP(maxfd,&fds_finished), 
	    FD_POP(maxfd,&ids_sleeping), 
	    FD_POP(maxfd,&ids_want_to_run), 
	    (FD_POP(maxfd,&fds_new)+ 
	     FD_POP(maxfd,&fds_active)+ 
	     FD_POP(maxfd,&fds_finished)+ 
	     FD_POP(maxfd,&ids_sleeping)+ 
	     FD_POP(maxfd,&ids_want_to_run))); 


	for(s=0; 
	    ((FD_POP(maxfd,&ids_want_to_run)>0) && 
	     (client_id = FD_FFSandC(s, maxfd, &ids_want_to_run)));
	    s = client_id+1)
	{
	    int new_fd;
	    
	    /* use client_id to dsitribute it over the listener pool */
	    new_fd = create_tx_tcp(state, ipaddr,
                                   base_rx_port+(client_id%num_ports));

	    if(new_fd < 0)
	    {
		fprintf(stderr,"Unable to open connection!\n");
		FD_SET(client_id, &ids_want_to_run);
		return(-1);


	    }else{
	    
		if(new_fd > maxfd) 
		    maxfd = new_fd;

		state[new_fd].client_id=client_id;
		state[new_fd].tx_target = objsize_distn(alldistns);

#ifdef DEBUG_DISTN
		fprintf(stderr,"%s: the objsize_distn returns %u \n",__FUNCTION__,
                        state[new_fd].tx_target);
#endif

		state[new_fd].rx_rcvd = 0;
		FD_SET(new_fd, &fds_new);
	    }
	}

	gettimeofday(&current_time,NULL);
    }


    if(verbose)
    fprintf(stdout,
            "%d.%06d new %d active %d finished %d sleeping %d want %d (%d)\n",  
	    (int)current_time.tv_sec,(int)current_time.tv_usec, 
	    FD_POP(maxfd,&fds_new), 
	    FD_POP(maxfd,&fds_active), 
	    FD_POP(maxfd,&fds_finished), 
	    FD_POP(maxfd,&ids_sleeping), 
	    FD_POP(maxfd,&ids_want_to_run), 
	    (FD_POP(maxfd,&fds_new)+ 
	     FD_POP(maxfd,&fds_active)+ 
	     FD_POP(maxfd,&fds_finished)+ 
	     FD_POP(maxfd,&ids_sleeping)+ 
	     FD_POP(maxfd,&ids_want_to_run))); 



    for(i=0; i <= maxfd; i++)
    {
	if(state[i].open)
	{
	    if(verbose) 
		printf("closing %d\n",i);
	    close(i);
	    state[i].open = 0;
	}
    }

    {
	int fd,i;
	struct timeval tdiff;
	for(i=0;(fd = FD_FFS(i, maxfd, &fds_active));
	    i = fd+1)
	{
	    tvsub(&tdiff, &state[fd].last_active, &state[fd].rx_start);
	    stats->partial++;
	    stats->tbytes += (state[fd].rx_rcvd);
	    stats->ttime_usec += (double)(tdiff.tv_sec*1.e6 + tdiff.tv_usec);
	}    
    }







    return(0);
}



/*********************************************************************
 * distributions used by surge_clt things 
 */
double 
constant(distn *d)
{
    return (d->mean);

}

double
expon(distn *d)
{
    double r=(double) drand48();
    double val= -log(r) * d->mean;

#ifdef DEBUG_DISTN
    fprintf(stderr,"%s: returns %g\n",__FUNCTION__,val);
#endif

    return (val);
    
}

double
paretoon(distn *d)
{
    double r = (double) drand48();
    /* a variation of this I use in my cell generator, however here we
     * use the `by the book' ns function.
     *
     * scale * (1.0/pow(uniform(), 1.0/shape))
     *
     * double val = ((1/mean) * (shape - 1)/shape) * ( 1 / pow(r,(1/shape)));
     */

    double val = (d->mean * (1.0/pow(r, 1.0/d->shape)));

    return (val);
}


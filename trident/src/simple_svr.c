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
#include <unistd.h>


#include "trc.h"
#include "trident.h"
#include "trident_util.h"

#define INACTIVE_TIMEOUT 10000000l
/* (10seconds in us)*/


/*********************************************************************
 * open connections and send requested amt of data; estimate b/w
 */
void
simple_server(int num_ports, int base_rx_port)
{
    state_t state[__FD_SETSIZE];
    int datafd, maxlfd,s,fd ;
    unsigned long diffus;
    struct timeval last, diff, now ={ 0L, 0L };
    int opened=0, closed=0, cnt=0; 
    fd_set fds_listeners, fds_active, fds_finished;

    int rc;

    FD_ZERO(&fds_listeners);
    FD_ZERO(&fds_active);
    FD_ZERO(&fds_finished);

    /* listeners on num_ports */
    if((maxlfd = create_listeners(state, &fds_listeners, num_ports, base_rx_port)) < 0)
    {
      fprintf(stderr,"%s: create_listeners returned <0\n",__FUNCTION__);
	exit(-1);
    }

    datafd = maxlfd+1;

    while(1)
    {
	/* grab any new incoming connections */
	rc = accept_incoming(state, maxlfd, &fds_listeners, &fds_active);
	if(rc > 0)
	{
	    opened += rc;
	}
	/*  select on the data FD's */
	rc = send_data(state, &fds_active, &fds_finished);
	if(rc > 0)
	{
	    closed += rc;
	}
	cnt++; 
	 
	gettimeofday(&now, (struct timezone *)0);
	
	tvsub(&diff, &now, &last);
	diffus = diff.tv_sec*1e6 + diff.tv_usec;
	
	if(diffus > SAMPLE_PERIOD)
	{
	    int i, totb=0, prog=0;
	    double mbs, tmbs=0.0;

	    fprintf(stderr, "\nBandwidth:\n");

	    for(i=datafd; i <= maxfd; i++)
	    {
		if(state[i].tx_sent_cpt) 
		{
		    prog++;
		}
		totb += state[i].tx_sent_cpt;
		mbs   = (double)(8.0*state[i].tx_sent_cpt) / (double)diffus;
		tmbs += mbs;
	      
		if(state[i].open) 
		{
		    fprintf(stderr,"%c%.4f ",'+', mbs);
		}
		else
		{
		    if(verbose)
		    {
			fprintf(stderr,"%c%.4f ",'-', mbs);
		    }
		}

		state[i].tx_sent_cpt = 0;


	    }


	    for(s=0;(fd = FD_FFS(s, maxfd, &fds_active));
		s = fd+1) {
		tvsub(&diff, &now, &state[fd].last_active);
		diffus = diff.tv_sec*1e6 + diff.tv_usec;
#if 0
		fprintf(stderr,__FUNCTION__
			": %d diffus %d now %ld.%06ld last %ld.%06ld\n",
			fd,(int)diffus,
			now.tv_sec,now.tv_usec,
			state[fd].last_active.tv_sec,
			state[fd].last_active.tv_usec);
#endif /* 0 */
		if(diffus > INACTIVE_TIMEOUT) {

		    FD_CLR(fd, &fds_active);
		    close(fd);
		    state[fd].open = 0;
		    FD_SET(fd, &fds_finished);

		}

	    }




	    fprintf(stderr, 
		    "\n\t %d streams active(%d %d), %d made progress: "
		    "tot = %d, tot Mb/s = %.2f\n"
		    "\t opened %d, closed %d descriptors (loop count %d)\n\n",
		    FD_POP(maxfd, &fds_active), 
		    FD_POP(maxfd, &fds_finished), 
		    FD_POP(maxfd, &fds_listeners), 

		    prog, 
		    totb, tmbs/scalar, 
		    opened, closed, cnt);
	    if(verbose)
	    {
		int i;
		fprintf(stderr,"active ");
		for(i=0;i<maxfd;i++) {
		    if(FD_ISSET(i,&fds_active)) {
			fprintf(stderr,"%d,",i);
		    }
		}
		fprintf(stderr,"\n");

		fprintf(stderr,"listen ");
		for(i=0;i<maxfd;i++) {
		    if(FD_ISSET(i,&fds_listeners)) {
			fprintf(stderr,"%d,",i);
		    }
		}
		fprintf(stderr,"\n");

		fprintf(stderr,"finished ");
		for(i=0;i<maxfd;i++) {
		    if(FD_ISSET(i,&fds_finished)) {
			fprintf(stderr,"%d,",i);
		    }
		}
		fprintf(stderr,"\n");
	    }
    
	    opened = closed = cnt = 0;
	    last = now; 
	}
    } /* end of while 1 */
}

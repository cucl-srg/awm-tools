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

/*********************************************************************
 * create listeners and sink tx'd data
 */
void
simple_rx(int num_ports, int base_rx_port)
{
    state_t state[__FD_SETSIZE];


    int datafd, maxlfd;
    unsigned long diffus;
    struct timeval last, diff, now={ 0L, 0L };
    int opened=0, closed=0, cnt=0; 
    fd_set fds_listeners, fds_active, fds_finished;

    int s, rc, fd;

    FD_ZERO(&fds_listeners);
    FD_ZERO(&fds_active);
    FD_ZERO(&fds_finished);

    /* listners on num_ports */
    if((maxlfd = create_listeners(state, &fds_listeners, num_ports, base_rx_port)) < 0)
    {
      fprintf(stderr,"%s:create_listeners < 0\n",__FUNCTION__);
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

	/* select on the data FDs */
	rc = sink_data(state, &fds_active, &fds_finished);
	if(rc > 0)
	{
	    closed += rc;
	}
	cnt++; 
		
	/* close those that have finished */
	for(s=0; 
	    (rc>0) && (fd = FD_FFSandC(s, maxfd, &fds_finished)); 
	    s = fd+1)
	{
	    close(fd);
	    state[fd].open = 0;
	    FD_CLR(fd, &fds_finished);
	}
	
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
		if(state[i].rx_rcvd_cpt)
		{
		    prog++;
		}
		totb += state[i].rx_rcvd_cpt;
		mbs   = (double)(8.0*state[i].rx_rcvd_cpt) / (double)diffus;
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
		state[i].rx_rcvd_cpt = 0;
	    }

	    fprintf(stderr, 
		    "\n\t %d streams active, %d made progress: "
		    "tot = %d, tot Mb/s = %.2f\n"
		    "\t opened %d, closed %d descriptors (loop count %d)\n\n",
		    FD_POP(maxfd, &fds_active), prog, 
		    totb, tmbs/scalar, 
		    opened, closed, cnt);
	    opened = closed = cnt = 0;
	    last = now;
	}
    } /* end of while 1 */
}

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
#include <sys/time.h>
#include <unistd.h>


#include "trc.h"
#include "trident.h"
#include "trident_util.h"


/*********************************************************************
 * send `n' conns. of `bytes' traffic to rx; keep trying to reopen
 * sockets as required, until done
 */
void
simple_tx(int n, int bytes, char *host, int num_ports, int base_rx_port)
{
    int i, left;
    fd_set fds_active, fds_finished;
    unsigned long ipaddr;
    double tmbs = 0;
    state_t state[__FD_SETSIZE];


    ipaddr = host2ipaddr(host);

    FD_ZERO(&fds_active);
    FD_ZERO(&fds_finished);

    /* create all the TCP conenctions */
    for(i=0; i<n; i++)
    {
	int fd = create_tx_tcp(state, ipaddr, base_rx_port+(i%num_ports));
	if(fd < 0)
	{
	    fprintf(stderr, "Unable to open connection!\n");
	}
	if(fd > maxfd) 
	{
	    maxfd = fd;
	}
	state[fd].tx_target = bytes;
	state[fd].tx_sent   = 0;
	state[fd].tx_pkts   = 0;
	gettimeofday(&state[fd].tx_start, (struct timezone *)0);
	FD_SET(fd, &fds_active);
    }

    /* start sending */
    left = -1;
    while(left)
    {
	int s, fd, fin;
	struct timeval tdiff;
	
	fin = send_data(state, &fds_active, &fds_finished);
	if(fin)
	{
	    fprintf(stderr, "%d finished\n", fin);
	}

	for(s=0; (fd = FD_FFSandC(s, maxfd, &fds_finished)); s = fd+1)
	{
	    double mbs = 0;

	    if(state[fd].tx_sent != state[fd].tx_target)
	    {
                /* we got closed before completing so try and set up
		 * again */
		int newfd = create_tx_tcp(state, ipaddr, base_rx_port+(fd%num_ports));
		if(newfd < 0)
		{
		    fprintf(stderr, "Unable to open connection!\n");
		}

		fprintf(stderr, 
			"Connection %d was aborted, so restart using fd %d\n",
			fd, newfd);

		if(newfd > maxfd) 
		{
		    maxfd = newfd;
		}
		
		state[newfd].tx_target = bytes;
		state[newfd].tx_sent   = 0;
		state[newfd].tx_pkts   = 0;
		gettimeofday(&state[fd].tx_start, (struct timezone *)0);

		FD_SET(newfd, &fds_active);
		continue;
	    }
	    
	    /* if we get here, connection must have completed its
	     * mission OK */
	    left = FD_POP(maxfd, &fds_active);
	    tvsub(&tdiff, &state[fd].tx_stop, &state[fd].tx_start);
	  
	    fprintf(stderr, 
		    "Finished with %d after %d bytes. ",
		    fd, state[fd].tx_sent);

	    if((tdiff.tv_sec == 0) && (tdiff.tv_usec == 0))
	    {
		mbs = 0;
	    }
	    else
	    {
		mbs = (double)(8.0*state[fd].tx_sent) / 
		    (double)(tdiff.tv_sec*1e6 + tdiff.tv_usec); 
		fprintf(stderr, 
			"%ld.%03lds => %.4f Mb/s. %d still active\n", 
		       (long int)tdiff.tv_sec, (long int)tdiff.tv_usec/1000,
		       mbs/scalar, left);
	    }
	    tmbs += mbs;
	    FD_CLR(fd, &fds_finished);
	}

    }
    fprintf(stderr, 
	    "Total b/w estimate was %.2f Mb/s, "
	    "Average stream b/w was %.4f Mb/s\n", tmbs, tmbs/n);
}

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
 * keep n connections open continuously; request `bytes' bytes each
 * time 
 */
void
continuous_client(int n, int bytes, char *host, int num_ports, int base_rx_port)
{
    int i;
    fd_set fds_new, fds_active, fds_finished;
    unsigned long ipaddr;
    double tmbs=0;
    state_t state[__FD_SETSIZE];

    ipaddr = host2ipaddr(host);

    FD_ZERO(&fds_new);
    FD_ZERO(&fds_active);
    FD_ZERO(&fds_finished);

    /* create all the TCP connections */
    for(i=0; i<n; i++)
    {
	int fd = create_tx_tcp(state, ipaddr, base_rx_port+(i%num_ports));
	struct timeval tp={ 0, 0 };

	if(fd < 0)
	{
	    printf("Unable to open connection!\n");
	    exit(-1);
	}
	if(fd > maxfd) 
	{
	    maxfd = fd;
	}

	state[fd].tx_target = bytes;
	state[fd].tx_sent   = 0;
	state[fd].tx_pkts   = 0;
	state[fd].rx_rcvd   = 0;
	FD_SET(fd, &fds_new);

	tp.tv_usec = rand()%10000;

	select(0, &fds_zero, &fds_zero, &fds_zero, &tp);
    }

    /* start sending */
    while(1)
    {
	int s, fd, rc;
	struct timeval tdiff;

	if( send_request(state, &fds_new, &fds_active) < 0 ) {
	  fprintf(stderr,"%s: send_request returned error\n",__FUNCTION__);
	    return;
	}

	rc = sink_data(state,&fds_active, &fds_finished);

	/* close those that have finished */
	for(s=0; 
	    (rc>0) && (fd = FD_FFSandC(s, maxfd, &fds_finished));
	    s = fd+1)
	{
	    double mbs;
	    int new_fd;

	    tvsub(&tdiff, &state[fd].rx_stop, &state[fd].rx_start);
	  
	    mbs = (double)(8.0*state[fd].rx_rcvd) /
		(double)(tdiff.tv_sec*1.e6 + tdiff.tv_usec);

	    tmbs += mbs;

	    fprintf(stdout, 
		    "Finished with %d after %d bytes. %ld.%03lds = %.4f "
		    "Mb/s.\n", 
		    fd, state[fd].rx_rcvd, 
		    (long int)tdiff.tv_sec, (long int)tdiff.tv_usec/1000, 
		    mbs/scalar);

	    close(fd);
	    state[fd].open = 0;

	    FD_CLR(fd, &fds_finished);

	    /* use fd to dsitribute it over the listener pool */
	    new_fd = create_tx_tcp(state, ipaddr, base_rx_port+(fd%num_ports));

	    if(new_fd < 0)
	    {
		fprintf(stderr, "Unable to open connection!\n");
		continue;
	    }
	  
	    if(new_fd > maxfd) 
	    {
		maxfd = new_fd;
	    }
	  
	    state[new_fd].rx_rcvd = 0;
	    FD_SET(new_fd, &fds_new);
	} 
    }
}

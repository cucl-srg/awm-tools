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
 * simple client (<->server); not used by any mode
 */
void
simple_client(int n, int bytes, char *host, int num_ports, int base_rx_port)
{
    state_t state[__FD_SETSIZE];
    int i, left;
    fd_set fds_new, fds_active, fds_finished;
    unsigned long ipaddr;
    double tmbs=0;

    ipaddr = host2ipaddr(host);

    FD_ZERO(&fds_new);
    FD_ZERO(&fds_active);
    FD_ZERO(&fds_finished);

    /* create all the TCP conenctions */
    for(i=0; i<n; i++)
    {
	int fd = create_tx_tcp(state, ipaddr, base_rx_port+(i%num_ports));
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
	FD_SET(fd, &fds_new);
    }

    /* start sending */
    left = -1;
    while(left)
    {
	int s, fd, rc;
	struct timeval tdiff;

	if( send_request(state, &fds_new, &fds_active) < 0 ) {
	  fprintf(stderr,"%s: send_request returned error\n",__FUNCTION__);
	    return;
	}
	rc = sink_data(state, &fds_active, &fds_finished);

	/* close those that have finished */
	for(s=0; 
	    (rc>0) && (fd = FD_FFSandC(s, maxfd, &fds_finished)); 
	    s = fd+1)
	{
	    double mbs;

	    left = FD_POP(maxfd, &fds_active);

	    /* rx_start/stop: when the CLIENT starts/stops receiving */
	    tvsub(&tdiff, &state[fd].rx_stop, &state[fd].rx_start);
	  
	    mbs = (double)(8.0*state[fd].rx_rcvd) / 
		(double)(tdiff.tv_sec*1.e6 + tdiff.tv_usec);

	    tmbs += mbs;

	    fprintf(stdout, 
		    "Finished with %d after %d bytes (%d pkts). "
		    "%ld.%03lds = %.4f Mb/s. "
		    "%d active\n", 
		    fd, state[fd].rx_rcvd, state[fd].rx_pkts,
		    (long int)tdiff.tv_sec, (long int)tdiff.tv_usec/1000, 
		    mbs/scalar, left);

	    close(fd);
	    state[fd].open=0;
	    FD_CLR(fd, &fds_finished);
	} 
    }
    fprintf(stdout, 
	    "Rough total b/w estimate was %.2f Mb/s, "
	    "Average stream b/w was %.4f Mb/s\n", tmbs, tmbs/n);
}

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


#include "trc.h"
#include "trident.h"
#include "trident_util.h"
#include "surge_clt.h"

/*********************************************************************
 * Global variables 
 */
/*state_t      state[ __FD_SETSIZE ];*/
fd_set       fds_zero;

int          maxfd     = 0;
trident_t    mode      = unset;
unsigned int base_port = 5001;
unsigned int num_ports = 32;
double       scalar    = 1.0;
int          verbose   = 0;

const struct timeval tsmallerpause = { 0, 1 };    /* 1us */
const struct timeval tpause        = { 0, 1000 }; /* 1ms */
const struct timeval tzero         = { 0, 0    }; 


/*********************************************************************
 * handle ^C (SIGINT) 
 */
void 
shut_sockets(int signum)
{
    int i;

    if(signum == SIGALRM) 
    {
	/* */
    }

	if(verbose) {
      printf("Shutdown called\n");
    }
#if 0
    for(i=0; i <= maxfd; i++)
    {
	if(state[i].open)
	{
	    fprintf(stderr, "%d ",i);
	    close(i);
	}
    }
#endif /* 0 */

    fprintf(stderr, "\n");


    exit(0);

}

/*********************************************************************
 * create listening sockets on [base_rx_port, base_rx_port+num_ports] 
 */
int 
create_listeners(state_t      *state,
		 fd_set *fds_listeners, int num_ports, int base_rx_port)
{
    int i, fd, maxfd=0;
    int on = 1;   /*  1 = non blocking  */

    for(i=0; i<num_ports; i++)
    {
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
	    perror("socket");
	    return -1;
	}

	state[fd].open = 1;

	bzero((char *)&state[fd].sinme, sizeof(state[fd].sinme));
	state[fd].sinme.sin_port =  htons(base_rx_port+i);
      
#if 0
	/* 6 = TCP */
	if(setsockopt(fd, 6, SO_REUSEADDR, &on, sizeof(int)) < 0)
	{
	    perror("SO_REUSEADDR");
	    return -1;	  
	}
#endif

	if(bind(fd, &state[fd].sinme, sizeof(state[fd].sinme)) < 0)
	{
	    perror("bind");
	    return -1;
	}
	if(ioctl(fd, FIONBIO, (char*)&on) < 0) 
	{
	    perror("FIONBIO");
	    return -1;
	}
	if(listen(fd, 16) < 0)
	{
	    perror("listen");
	    return -1;
	}
      
	FD_SET(fd, fds_listeners);

	if(fd > maxfd) 
	{
	    maxfd = fd;
	}
    }
    return maxfd;
}

/*********************************************************************
 * accept incoming socket requests, catching any exceptions
 */
int
accept_incoming(state_t *state,
		int maxlfd, fd_set *fds_listeners, fd_set *fds_active)
{
    int s, rc, fd, new_fd, count=0;
    fd_set fds_tmp1, fds_tmp2;
    struct timeval tmp_timeout;
    int on = 1;   /* 1 = non blocking */

    tmp_timeout = tzero;
			 
    fds_tmp1 = *fds_listeners;
    fds_tmp2 = *fds_listeners;

    /* zero timeout */
    rc = select(maxlfd+1, &fds_tmp1, &fds_zero, &fds_tmp2, &tmp_timeout);
  
    for(s=0; (rc>0) && (fd = FD_FFSandC(s, maxlfd, &fds_tmp2)); s = fd+1)
    {
	fprintf(stderr, "listen: got an exception on fd %d !!\n", fd);
	return -2;
    }
    
    /* accept any new requests */
    for(s=0; (rc>0) && (fd = FD_FFSandC(s, maxlfd, &fds_tmp1)); s = fd+1)
    {
	struct sockaddr_in frominet;
	int fromlen;

	fromlen = sizeof(frominet);      
	if((new_fd = accept(fd, &frominet, &fromlen)) < 0)
	{
	    perror("accept");
	    return -1;
	}
	else
	{
	    if(verbose)
	    {
		fprintf(stderr, "[ accept new_fd %d from fd %d ]\n", new_fd, fd);
	    }
	    if(new_fd > maxfd) 
	    {
		maxfd = new_fd;
	    }
	    if(ioctl(new_fd, FIONBIO, (char*)&on) < 0) 
	    {
		perror("FIONBIO");
		return -1;
	    }
	  
	    state[new_fd].sinme     = state[fd].sinme;
	    state[new_fd].sinhim    = frominet;

	    state[new_fd].rx_rcvd   = 0;
	    state[new_fd].rx_pkts   = 0;

	    state[new_fd].tx_sent   = 0;
	    state[new_fd].tx_target = 0;
	    state[new_fd].tx_pkts   = 0;

	    state[new_fd].open    = 1;


	    gettimeofday(&state[new_fd].last_active, (struct timezone *)0);

	    if(mode == rx)
	    {
		state[new_fd].rx_start=state[fd].last_active;
	    }




	    FD_SET(new_fd, fds_active);
	    count++;
	}	 
    }  
    return count;
}

/*********************************************************************
 * catch exceptions (=> drop socket) and suck data from those with
 * data waiting 
 */
int 
sink_data(state_t *state,
	  fd_set *fds_active, fd_set *fds_finished)
{
    int s, sel_rc, fd, fin = 0;
    fd_set fds_tmp1, fds_tmp2;
    struct timeval tmp_timeout;
    char buf[BUF_SIZE];

    tmp_timeout = tpause;

    fds_tmp1 = *fds_active;
    fds_tmp2 = *fds_active;

    /* 1ms timeout  */
    sel_rc = select(maxfd+1, &fds_tmp1, &fds_zero, &fds_tmp2, &tmp_timeout);
   
    /* check for exceptions */
    for(s=0; (sel_rc>0) && (fd = FD_FFSandC(s, maxfd, &fds_tmp2)); s = fd+1)
    {
	/* this shouldn't happen... */
	fprintf(stderr, 
		"rx data: got EXCEPTION on fd %d after %d bytes (%d pkts)\n", 
		fd, state[fd].rx_rcvd, state[fd].rx_pkts);
	close(fd);
	state[fd].open = 0;
	FD_CLR(fd, fds_active);
    }

    /* read those that are ready */
    for(s=0; (sel_rc>0) && (fd = FD_FFSandC(s, maxfd, &fds_tmp1)); s = fd+1)
    {
	int recv_rc;

	while(1)
	{
	    recv_rc = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);
	    if(recv_rc < 0)
	    {

		if(errno == EHOSTUNREACH || errno == ECONNREFUSED || 
		   errno == ECONNRESET || errno == ETIMEDOUT)
		{
		  fprintf(stderr,"%s (%d):errno%d (nonfatal)\n",__FUNCTION__, __LINE__, errno);
		    perror("Read");
		    
		}
		else 
		    if(errno != EWOULDBLOCK )
		    {
		      fprintf(stderr,"%s (%d):errno%d\n",__FUNCTION__, __LINE__,
				errno);
			perror("Read");
			exit(1);
		    }
		break;
	    }
	    else if(recv_rc == 0)
	    {
		/* EOF => tx has just closed connection */
		gettimeofday(&state[fd].rx_stop, (struct timezone *)0);
		FD_SET(fd, fds_finished);
		FD_CLR(fd, fds_active);
		fin++;
		break;
	    }
	    else /* recv_rc > 0 */
	    {
		state[fd].rx_rcvd     += recv_rc;
		state[fd].rx_rcvd_cpt += recv_rc;
		state[fd].rx_pkts++;
		gettimeofday(&state[fd].last_active, (struct timezone *)0);
	    }
	}	  	  
    }
    return fin;
}

/*********************************************************************
 * client request amt of data from server; marks connection new ->
 * active 
 */
int
send_request(state_t *state,
	     fd_set *fds_new, fd_set *fds_active)
{
    struct timeval tmp_timeout;
    int rc, fd, s;
    fd_set fds_tmp1, fds_tmp2;


    fds_tmp1 = fds_tmp2 = *fds_new;
    tmp_timeout = tzero;

    rc = select(maxfd+1, &fds_zero, &fds_tmp1, &fds_tmp2, &tmp_timeout);

    if(rc < 0)
    {	
	perror("select");
	goto abort;
    }

    if(!rc)
	goto calmExit;

    /* check for exceptions first */
    for(s=0; (fd = FD_FFSandC(s, maxfd, &fds_tmp2)); s = fd+1)
    {
	if(verbose) 
	    printf("[ send request: got an exception on fd %d ]\n", fd);
    }
	
    if(s)
	goto abort;

    /* check for fds ready to write */
    for(s=0; (fd = FD_FFSandC(s, maxfd, &fds_tmp1)); s = fd+1)
    {
	u_int32_t bytes = state[fd].tx_target;

	gettimeofday(&state[fd].rx_start, (struct timezone*)0);
	state[fd].last_active=state[fd].rx_start;


	rc = write(fd, &bytes, sizeof(u_int32_t));
	if(rc != sizeof(u_int32_t) && verbose)
	{
	    printf("[ send request: write on %d got %d ]\n", fd, rc);
	    if(rc < 0)
		perror("send request write");
	}


	FD_CLR(fd, fds_new);
	FD_SET(fd, fds_active);
    }

calmExit:
    return(0);
abort:
    return(-1);

}

/*********************************************************************
 * svr: (server) (send_data) rx request -> 
 *                        (send_data) tx amt of data (miss FIN/ACK RTT)
 * tx : (send_simple_traffic) successful open conn. -> 
 *                        (send_data) tx amt of data (miss FIN/ACK RTT)
 *
 * clt: (continuous_client) (send_request) send rx request -> 
 *                        (sink_data) read EOF from sock.
 * rx : (rx_sink) (accept_incoming) accept connection ->
 *                        (sink_data) read EOF from sock.
 *********************************************************************
 */

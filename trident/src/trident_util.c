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
#include <string.h>

extern char   *prog_name;

#include "trc.h"
#include "trident.h"
#include "trident_util.h"

/*********************************************************************
 * Error printing
 */
void
fatal(char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    fprintf(stderr, "%s FATAL: ", prog_name);
    vfprintf(stderr, format, ap);
    if(*format && format[strlen(format) - 1] != '\n')
    {
	fprintf(stderr, "\n");
    }
    fflush(stderr);
    va_end(ap);

    exit(-1);
}

void
warning(char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    fprintf(stderr, "%s WARNING: ", prog_name);
    vfprintf(stderr, format, ap);
    if(*format && format[strlen(format) - 1] != '\n')
    {
	fprintf(stderr, "\n");
    }
    fflush(stderr);
    va_end(ap);
}

/*********************************************************************
 * Parse string -> xxx
 */
int
getint(char *str)
{
    long l;
    char *str2;

    l = strtol(str, &str2, 10);
    if(str == str2)
    {
	fatal("%s: not an integer", str);
    }
    return (int)l;
}

double
getdouble(char *str)
{
    double d;
    char *str2;

    d = strtod(str, &str2);
    if(str == str2)
    {
	fprintf(stderr, "%s: not a double", str);
	exit(-1);
    }
    return (double) d;
}

/*********************************************************************
 * resolve IP address/name to 0xXXXXXXXX format
 */
unsigned long
host2ipaddr(const char* str)
{
    struct in_addr inaddr;
    struct hostent *host;

    ENTER;
    TRC("%s\n", str);

    if(!(host = gethostbyname(str)))
    {
      fprintf(stderr,"%s:fatal\n",__FUNCTION__);
	herror(prog_name);
	exit(-1);
    }
    inaddr.s_addr = *(uint32_t*)host->h_addr;

    TRC("%d\n", inaddr.s_addr);
    RETURN inaddr.s_addr;
}
/* if(t1 <= t0) return 1 else return 0 */
int
tveqless(struct timeval *t1, struct timeval *t0)
{
    if((t1->tv_sec < t0->tv_sec) ||
       ((t1->tv_sec == t0->tv_sec) && 
	(t1->tv_usec <= t0->tv_usec)))
    {
	return 1;
    }
    else
    {
	return 0;
    }
}

/* if(t1 < t0) return 1 else return 0 */
int
tvless(struct timeval *t1, struct timeval *t0)
{
    if((t1->tv_sec < t0->tv_sec) ||
       ((t1->tv_sec == t0->tv_sec) && 
	(t1->tv_usec < t0->tv_usec)))
    {
	return 1;
    }
    else
    {
	return 0;
    }
}

/* safe timeval add: t1+t0 */
void
tvadd(struct timeval *tdiff, struct timeval *t1, struct timeval *t0)
{
    tdiff->tv_sec = t1->tv_sec + t0->tv_sec;
    tdiff->tv_usec = t1->tv_usec + t0->tv_usec;
    if(tdiff->tv_usec >= 1e6)
	tdiff->tv_sec++, tdiff->tv_usec -= 1e6;
}

/* safe timeval subtract: t1-t0 */
void
tvsub(struct timeval *tdiff, struct timeval *t1, struct timeval *t0)
{
    tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
    tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
    if(tdiff->tv_usec < 0)
	tdiff->tv_sec--, tdiff->tv_usec += 1e6;
}

/*********************************************************************
 * Population count functions
 */

/* count 1s ("population") in (binary) w */
inline unsigned int
pop_count(unsigned int w)
{
    unsigned int res = (w & 0x55555555) + ((w >> 1) & 0x55555555);
    res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
    res = (res & 0x0F0F0F0F) + ((res >> 4) & 0x0F0F0F0F);
    res = (res & 0x00FF00FF) + ((res >> 8) & 0x00FF00FF);
    return (res & 0x0000FFFF) + ((res >> 16) & 0x0000FFFF);
}

/* population count, noting that fd set may be more than a single
 * unsigned int in size */
int
FD_POP(int maxfd, fd_set *fd)
{
    int i, j=0;

    for(i=0; i <= maxfd / __NFDBITS; i++)
    {
	__fd_mask x = fd->__fds_bits[i];

	if(i == maxfd / __NFDBITS)
	{
	    x &= ~((-2L) << (maxfd % __NFDBITS));
	}
	j += pop_count(x);
    }
    return j;
}

/* find first 1 in (binary) fd set after `start' */
int 
FD_FFS(int start, int maxfd, fd_set *fd)
{
    int i,j;

    for(i = start / __NFDBITS; i <= maxfd / __NFDBITS; i++)
    {
	__fd_mask x = fd->__fds_bits[i];

	if(start % __NFDBITS)
	{
	    x = x & (~((u_int32_t)0)) <<  (start % __NFDBITS);
	    start = 0;
	}	    

	if(x == 0) 
	    continue;  /* nothing set here */

	j = ffs_long((u_int32_t)x) - 1 + i*__NFDBITS;

	if(j > maxfd) 
	    return 0;
	else
	    return j;	
    }
    return 0;
}

/* the following assumes all bits before `start' are clear, and clears
 * the bit it finds from the fd set; i.e. as above, but clear that bit */
int 
FD_FFSandC(int start, int maxfd, fd_set * fd)
{
    int i,j;

    for(i = start / __NFDBITS; i <= maxfd / __NFDBITS; i++)
    {
	__fd_mask x = fd->__fds_bits[i];

	if(x == 0)
	    continue;  /* nothing set here */

	j = ffs_long((u_int32_t) x) - 1 + i*__NFDBITS;

	if(j > maxfd) 
	    return 0;
	else
	{
	    FD_CLR(j, fd);
	    return j;	
	}
    }
    return 0;
}

/* create required tx connection */
int
create_tx_tcp(state_t *state,
	      unsigned long d_ipaddr, int d_port)
{
    int fd;
    int on = 1;   /* 1 = non blocking */

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	perror("socket");
	return -1;
    }

    state[fd].open = 1;

    bzero((char *)&state[fd].sinme, sizeof(state[fd].sinme));
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

    bzero((char *)&state[fd].sinhim, sizeof(state[fd].sinhim));
    state[fd].sinhim.sin_family      = AF_INET;
    state[fd].sinhim.sin_port        = htons(d_port);
    state[fd].sinhim.sin_addr.s_addr = d_ipaddr;

    if(connect(fd, &state[fd].sinhim, sizeof(state[fd].sinhim)) < 0)
    {
	if(errno != EINPROGRESS)
	{
	    perror("connect");      
	    return -1;
	}
    }
    return fd;
}

/* catch exceptions, and send data (either how much tx wants, or (if
 * server) how much client requested) */
int
send_data(state_t *state,
	  fd_set *fds_active, fd_set *fds_finished)
{
    struct timeval tmp_timeout, d;
    int sel_rc, fd, s, fin=0;
    char buf[BUF_SIZE];
    
    fd_set fds_tmp0, fds_tmp1, fds_tmp2;

    fds_tmp0 = fds_tmp1 = fds_tmp2 = *fds_active;
    tmp_timeout = tsmallerpause;

    sel_rc = select(maxfd+1, &fds_tmp0, &fds_tmp1, &fds_tmp2, &tmp_timeout);
    if(sel_rc < 0 && errno != EAGAIN)
    {	
	perror("select");
	return(-1);
    }
    else if(sel_rc == 0 || errno == EAGAIN)
    {
	return 0;   /* nothing to do */
    }

    /* check for exceptions first */
    for(s=0; (fd = FD_FFSandC(s, maxfd, &fds_tmp2)); s = fd+1)
    {
	printf("Got an exception on fd %d\n", fd);
    }
	
    if(s)
    { 
	return(-1);
    }

    /* check for any ready to read */
    for(s=0; (fd = FD_FFSandC(s, maxfd, &fds_tmp0)); s = fd+1)
    {
	int rc;
	u_int32_t bytes;
	
	rc = read(fd, &bytes, sizeof(u_int32_t));
	if(rc == sizeof(u_int32_t))
	{
	    if(state[fd].tx_target != state[fd].tx_sent)
	    {
		printf("[ warning: premature request on %d ]\n", fd);
	    }
	    if(verbose)
		printf("[ request for %d bytes on fd %d ]\n", bytes, fd);

	    state[fd].tx_target = bytes;
	    state[fd].tx_sent   = 0;
	    state[fd].tx_pkts   = 0;
	    /* set tx_start if we are server */
	    gettimeofday(&state[fd].last_active, (struct timezone *)0);
	    if(mode == svr)
	    {
		state[fd].tx_start=state[fd].last_active;
	    }
	}
    }

    /* check for fds ready to write */
    for(s=0; (fd = FD_FFSandC(s, maxfd, &fds_tmp1)); s = fd+1)
    {
	int diff, len, actual;

	diff = state[fd].tx_target - state[fd].tx_sent;
	
	if(diff == 0) 
	    continue;

	len = MIN(diff, BUF_SIZE);

	/* could loop around this -- more efficient for smaller
	 * numbers of connections */

	actual = write(fd, buf, len);
	if(actual < 0)
	{
	    if(errno == EWOULDBLOCK)
	    {
		continue;
	    }
	    else if(errno == EPIPE)
	    {
		if(verbose)
		    fprintf(stderr, 
			    "[ fd %d received EPIPE; closing socket... ]\n", fd);
		if(state[fd].open)
		{
		    close(fd);
		    state[fd].open = 0;
		    state[fd].tx_stop = state[fd].tx_start;
		    FD_CLR(fd, fds_active);
		    FD_SET(fd, fds_finished);
		    fin++;
		}
		else
		{
		    fprintf(stderr, 
			    "[ fd %d received EPIPE whilst not open! ]\n", fd);
		}
	    }
	    else /* other errno */
	    {
		fprintf(stderr, "errno = %d; ", errno);
		perror("write");
		return(-1);
	    }
	}
	else if(actual == 0)
	{
	    printf("[ wrote zero to fd %d ]\n", fd);
	}
	else /* actual > 0 */
	{
	    state[fd].tx_sent     += actual;
	    state[fd].tx_sent_cpt += actual;
	    state[fd].tx_pkts++;

	    gettimeofday(&state[fd].last_active, (struct timezone *)0);	  

	    if(diff == actual)
	    { /* we're done */
		FD_CLR(fd, fds_active);
		close(fd);
		state[fd].open = 0;
		FD_SET(fd, fds_finished);

		gettimeofday(&state[fd].tx_stop, (struct timezone *)0);	    
		tvsub(&d, &state[fd].tx_stop, &state[fd].tx_start);
		if(verbose)
		    printf("[ finished sending %d bytes (%d pkts) to fd %d "
			   "in %lu.%03lus ]\n",
			   state[fd].tx_target, state[fd].tx_pkts, fd, 
			   d.tv_sec, d.tv_usec/1000);

		fin++;
	    }
	}
    }
    return fin;
}



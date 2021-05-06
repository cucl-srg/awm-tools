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



#ifndef _trident_h_
#define _trident_h_

#define __FD_SETSIZE 1024

typedef struct {
    struct sockaddr_in sinme;
    struct sockaddr_in sinhim;

    int open;

    u_int32_t      tx_target;   /* used by tx side                    */

    u_int32_t      tx_pkts;
    u_int32_t      tx_sent;
    u_int32_t      tx_sent_cpt; /* bytes rx'd since checkpoint        */
    struct timeval tx_start;    /* send_data(): just as start sending */
    struct timeval tx_stop;     /* send_data(): when all is sent      */

    u_int32_t      rx_pkts;
    u_int32_t      rx_rcvd;     /* used by rx side                    */
    u_int32_t      rx_rcvd_cpt; /* bytes rx'd since checkpoint        */
    struct timeval rx_start;    /* sink_data(): when first bits rcvd  */
    struct timeval rx_stop;     /* sink_data(): when no bits rcvd     */

    struct timeval last_active; /* time last active used in svr to close
				   dead/inactive connections */

    /* state for handling more complex traffic generators             */
    int            client_id;

    int            object_count;
    struct timeval next_start_time;    /* time until next wake up */
} state_t;


typedef struct {
    double ttime_usec;
    double tbytes;
    unsigned long conns;
    unsigned long partial; /* incomplete connection(s) that where active */
    struct timeval start_time;
} stats;


typedef enum {
    unset = -1, rx = 0, tx, svr, simple_clt, cts_clt, surge_clt
} trident_t;

struct _distn;

struct _distn {
    double (*function) (struct _distn *distribution); /* distribution function */
    double mean;     /* mean of distribution           */
    double shape;    /* shape of (pareto) distribution */

};
typedef struct _distn distn;

typedef struct {
    distn interpage;
    distn objperpage;
    distn interobj;
    distn objsize;
} alldistn;

/*********************************************************************
 * important constants 
 */
#define BUF_SIZE      1448
#define SAMPLE_PERIOD 5e6 /* in microsecs */


/*extern state_t state[__FD_SETSIZE];*/
extern fd_set  fds_zero;
extern int          maxfd;
extern trident_t      mode;
extern unsigned int base_port;
extern unsigned int num_ports;
extern double       scalar;
extern int          verbose;

extern const struct timeval tsmallerpause;    /* 1us */
extern const struct timeval tpause; /* 1ms */
extern const struct timeval tzero; 

/*********************************************************************
 * function prototypes
 */

int surge_client(int n, char *host, int num_ports, int base_rx_port, stats 
		 *stats, struct timeval *run_time,alldistn *alldistns);

void continuous_client(int n, int bytes, char *host, int num_ports, int base_rx_port);

void simple_tx(int n, int bytes, char *host, int num_ports, int
base_rx_port);

void simple_rx(int num_ports, int base_rx_port);

void simple_client(int n, int bytes, char *host, int num_ports, int
base_rx_port);

void simple_server(int num_ports, int base_rx_port);

void fileParser(int argc, char** argv);

int sink_data(state_t *state,
	      fd_set *fds_active, fd_set *fds_finished);
int send_request(state_t *state,
		 fd_set *fds_new, fd_set *fds_active);
int create_listeners(state_t *state,
		     fd_set *fds_listeners, int num_ports, 
                     int base_rx_port);

int accept_incoming(state_t *state,
		    int maxlfd, fd_set *fds_listeners, 
                    fd_set *fds_active);

void shut_sockets(int signum);



#endif /* _trident_h_ */

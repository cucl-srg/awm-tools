#include <sys/time.h>


/*
 * timeval compare macros
 */
#define tv_ge(lhs,rhs) (tv_cmp((lhs),(rhs)) >= 0)
#define tv_gt(lhs,rhs) (tv_cmp((lhs),(rhs)) >  0)
#define tv_le(lhs,rhs) (tv_cmp((lhs),(rhs)) <= 0)
#define tv_lt(lhs,rhs) (tv_cmp((lhs),(rhs)) <  0)
#define tv_eq(lhs,rhs) (tv_cmp((lhs),(rhs)) == 0)
/*  1: lhs >  rhs */
/*  0: lhs == rhs */
/* -1: lhs <  rhs */
int
tv_cmp(struct timeval lhs, struct timeval rhs)
{
    if (lhs.tv_sec > rhs.tv_sec) {
        return(1);
    }
	
    if (lhs.tv_sec < rhs.tv_sec) {
        return(-1);
    }
	
    /* ... else, seconds are the same */
    if (lhs.tv_usec > rhs.tv_usec)
        return(1);
    else if (lhs.tv_usec == rhs.tv_usec)
        return(0);
    else
        return(-1);
}



/* subtract the rhs from the lhs, result in lhs */
void
tv_sub(struct timeval *plhs, struct timeval rhs)
{
    /* sanity check, lhs MUST BE more than rhs */
    if (plhs->tv_usec >= rhs.tv_usec) {
        plhs->tv_usec -= rhs.tv_usec;
    } else if (plhs->tv_usec < rhs.tv_usec) {
        plhs->tv_usec += 1000000 - rhs.tv_usec;
        plhs->tv_sec -= 1;
    }
    plhs->tv_sec -= rhs.tv_sec;
}


/* return elapsed time in microseconds */
/* (time2 - time1) */
double
elapsed(
		struct timeval time1,
		struct timeval time2)
{
    struct timeval etime;
	
    /*sanity check, some of the files have packets out of order */
    if (tv_lt(time2,time1)) {
        return(0.0);
    }
		
    etime = time2;
    tv_sub(&etime, time1);
    
    return((double)etime.tv_sec * 1000000 + (double)etime.tv_usec);
}


struct netflowrecord
{
	long srcip;				// SOURCE IP ADDRESS
	unsigned int srcport;	// SOURCE PORT
	
	long dstip;				// DESTINATION IP ADDRESS
	unsigned int dstport;	// DESTINATION PORT
	
	unsigned int packets;	// NUMBER OF PACKETS IN FLOW
	unsigned long bytecount;// NUMBER OF BYTES IN FLOW
		
		struct timeval starttime;// FLOW START TIME
		struct timeval endtime;	// FLOW END TIME
			
			int flags;	
			int protocol;		
} _netflowrecord;

struct packetlinkedlist
{
	int SENT; // not actually sent, morelike notsent	
	struct timeval time;
	int flags;

	unsigned int length; // length of entire ip packet
	unsigned int data; // length of ip packets - tcp & ip headers
	unsigned int seq; //
	unsigned int ack;
	// WE CAN MAKE last_data_byte from seq + data - 1
	unsigned int RTT;
	unsigned int retrans;
	unsigned int retrans_diffdata;
	unsigned int outoforder;
	struct packetlinkedlist *packet;
	struct packetlinkedlist *prev;	
} _packetlinkedlist;


struct flow 
{	
	unsigned int syn;
	struct netflowrecord *n;			// WEAK ATTEMPT AT NETFLOWRECORD
	struct packetlinkedlist *packet;	// LINKEDLIST OF PACKETS
	struct packetlinkedlist *startpacket;
	int STATE;							// CURRENT TCP FLOW STATE
	struct timeval LASTPACKET;				// TIME OF LAST PACKET
	unsigned long key;					// KEY USED IN HASHMAP
	int CLASS_VALUE; // class value, -1 is unclassed
	int MID_STREAM_SAMPLE; // either a value of 0 or 1 depending on whether 
			       // it is a MID_STREAM_SAMPLE or not
	int COMPLETE; // did this flow terminate correctly
        int ABORTED; // has this flow been aborted already
	
} _flow;


// TCP flags
short FIN = 1;
short SYN = 2;
short RST = 4;
short PSH = 8;
short ACK = 16;
short URG = 32;
short ECN = 64;
short CWR = 128;

// TCP states
unsigned int CLOSED = 0;
unsigned int SYN_SENT = 1;
unsigned int SYN_RCVD = 2;
unsigned int ESTABLISHED = 3;
unsigned int CLOSE_WAIT = 4;
unsigned int LAST_ACK = 5;
unsigned int FIN_WAIT_1 = 6;
unsigned int FIN_WAIT_2 = 7;
unsigned int CLOSING = 8;
unsigned int TIME_WAIT = 9;
unsigned int CRASHED = 10; // used to drop RST packets



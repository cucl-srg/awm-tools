/**
The flow demuxer header

All a flow demuxer does is deal with packets
**/
#include <sys/time.h>
#include <stdio.h>
unsigned int NEWFLOWS = 0;
unsigned int FLOWSCOMPLETED = 0;
unsigned int FLOWSABORTED = 0;
unsigned int PACKETSSEEN = 0;
unsigned int TIMEOUT = 600000;
FILE * LOGFILE;

void setupdemuxer(); // setup classifier, create files, etc...

void streamfinished();

void dealwithpacket(long sip, long dip, int spo, int dpo, int flg
				  , int ack, int seq
				  , unsigned int data, unsigned int lgt
				  , struct timeval packtime);

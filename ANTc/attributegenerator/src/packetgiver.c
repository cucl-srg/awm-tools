/******************************************************************************
*                                                                             *
*   Copyright 2006 Queen Mary University of London                            *
*                                                                             *
*   This file is part of ANTc.                                                *
*                                                                             *
*   ANTc is free software; you can redistribute it and/or modify              *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   ANTc is distributed in the hope that it will be useful,                    *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with Trident; if not, write to the Free Software                    *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*                                                                             *
******************************************************************************/

#include "flowdemuxer.h"
#include "attributecalculator.h"
#include "hashtable.c"
#include "hashtable_private.h"
#include "mtflinkedlist.h"

/**
 * VARIABLES
 */
struct timeval LASTPACKET;  // LASPACKET SEEN
struct hashtable *flows;    // THE HASHTABLE FOR THE FLOW STRUCTS
struct mtflinkedlist *list; // THE LINKEDLIST FOR THE FLOW STRUCTS
/**
FUNCTIONS NEEDED FOR HASHTABLE
 **/

/**
 * Create a key using the 4-tuple (hosta,porta),(hostb,portb).
 * Order the variables so that whether its client->server server->client
 * the generated key is always the same.
 *
 * Bitshifting malarky taken from a good hashing function website online
 * (no reference sorry)
 */
unsigned long createflowkey(long srcip, long dstip, int srcport, int dstport)
{
	if (srcip < dstip)
		return (unsigned long)(((srcip << 17) | (srcip >> 15)) ^ dstip) +
            (srcport * 17) + (dstport * 13 * 29);
	else
		return (unsigned long)(((dstip << 17) | (dstip >> 15)) ^ srcip) +
            (dstport * 17) + (srcport * 13 * 29);
}


/**
 * This is an extension to the hashtable: originally it frees stored objects within 
 * the hashtable, however, due to our objects (flows) owning a whole set of other
 * objects which need to be freed, we also need to implement a custom remove method
 * as well.
 *
 * This procedure iterates through all the flow's packets and frees them, frees the
 * netflowrecord object and finally frees the flow
 */
void removeValue(void *value)
{
	struct flow *f = (struct flow *)value;
	/**
	REMEMBER: LOOP THROUGH THE PACKET LIST AND REMOVE ALL THE PACKETS AS WELL
	**/
	struct packetlinkedlist * tbfree;
	struct packetlinkedlist * p = f->startpacket;
	while (p != NULL)
		{
		tbfree = p;
		p = p->prev;
		free(tbfree);
		}
	free(f->n); // remove netflowrecord
	free(f); // remove the flow
}

/**
 * Another hashtable required function, however, as we use the actual hash key
 * as the key object, we just return the value.. i.e. rather than creating a 
 * 4-tuple object for each flow, we just look at the flow key each time 
 */
unsigned long flowhashfn (void *flowk)
{
	return ((unsigned long *)flowk);
}

/**
 * Are these two flows equal, simple unsigned long comparison
 */
int flowequalfn (void *flowval1, void *flowval2)
{
	return (unsigned long *)flowval1 == (unsigned long *)flowval2;
}

/**
 * END FUNCTIONS NEEDED FOR HASHTABLE
 */

/**
 * Create a new flow object and initialise as many variables as possible
 */
struct flow * createflow(int seq,long sip, long dip, int spo, int dpo,
				 	unsigned int key, unsigned int lgt)
{
	
	struct flow *newf = malloc(sizeof(struct flow));	
	bzero(newf,sizeof(struct flow));
	newf->n = malloc(sizeof(struct netflowrecord));
	bzero(newf->n,sizeof(struct netflowrecord));	
	newf->STATE = CLOSED;
	newf->packet = NULL;
 	newf->CLASS_VALUE = -1;	
	newf->LASTPACKET = LASTPACKET;
	newf->syn = seq;
	newf->n->srcip = sip;
	newf->n->srcport = spo; 
	newf->n->dstip = dip;
	newf->n->dstport = dpo;
	newf->key = key;
	newf->n->bytecount = lgt;	
	return newf;
}

/**
 * Compute whether a packet has been retranmitted or not.
 *
 * returns 1 or more if it has, or 0 if not.
 */
unsigned int retrans(unsigned int flg, unsigned int data, unsigned int ack, 
	unsigned int seq, struct flow *f, int SENT, struct packetlinkedlist *thepacket)
{
    struct packetlinkedlist *p = f->packet;


    if (p ==  NULL)
		{
		return 0;
		}
	
	for (; p != NULL; p = p->packet)
	{
		if (p->SENT != SENT)
			continue;
		if (seq < p->seq && (flg & SYN) != SYN)
		{
		
			for (; p != NULL; p = p->packet)
			{
				if (p->SENT != SENT)
					continue;
				if (p->seq == seq && p->data > 0)
					{
					return 1 + p->retrans;	
					}
			}

			return 0; // this is just an out of order packet
		}
		
		// special case of PSH FIN ACK being retransmitted
		if (p->flags == flg && p->data > 0 
			&& data == 0 && p->ack == ack && p->seq == seq - p->data)
		{
			return p->retrans + 1;		
		}
		
		if (p->retrans == 0 && p->seq < seq)
		{
			return 0;
		}
		// same seq, same data, must be retransmit
		if (p->seq == seq && (data==p->data || (p->ack == ack && p->data > 0)))
		{			
			if (p->flags == flg)
			{
				if (p->data != data && data != 0)
				{
					thepacket->retrans_diffdata = 1;
				}
				return 1 + p->retrans;
			}
		}
	}
	return 0;
}

/**
 * Add a new packetlinkedlist object to a flow object
 * This method is only called if dealwithpacket decides a packet should be added
 */ 
void addPacketToFlow(int SENT, struct flow *f, struct timeval packtime, unsigned int lgt, unsigned int data, int flg, unsigned int ack, unsigned int seq)
{
	struct packetlinkedlist *p;
	p = malloc(sizeof(struct packetlinkedlist));
	p->seq = seq;
	p->ack = ack;
	p->SENT = SENT;
	p->RTT = 0;
	p->retrans = 0;
	p->retrans_diffdata = 0;
	p->outoforder = 0;

	if ((flg & FIN) == FIN || (flg & SYN) == SYN || (data != 0))
		p->retrans = retrans(flg,data,ack,seq,f,SENT,p);	
	p->length = lgt;
	p->data = data;
	p->flags = flg;
	p->prev = NULL;
	p->packet = NULL;
	if (f->packet != NULL)
	{
		p->packet = f->packet;
		f->packet->prev = p;		
	}
	f->packet = p;
	p->time = packtime;
	
	/**	UPDATE TIME ORDERED LINKED LIST     **/
	mtflinkedlist_update(list,f->key);
	f->n->bytecount += lgt;		
	f->n->packets++;			
	
}


/** 
 * Set the flow state based on previous state and new flow flags
 */
void settcpconnectionstate(long srcip, struct flow * f, int flg, int seq)
{

	short SENT = 0;
	if (srcip == f->n->srcip){
		SENT = 1;
	}
	
	if ((flg & RST) == RST)
	{
		f->STATE = CRASHED;
		f->ABORTED = 1;
		return;
	}
	
	// CLOSED STATE
	if (f->STATE == CLOSED) // can only accept SYN or SYN ACK
	{
		if ((flg & SYN) == SYN) // found a SYN
		{
			if (SENT)
			{
				f->STATE = SYN_SENT;
			}
			else
			{
				f->STATE = SYN_RCVD;
			}
		}
		else
		{
			// NOT SYN - SYN/ACK so we just say this is a 
			// MID STREAM SAMPLE and ESTABLISH it
			f->MID_STREAM_SAMPLE = 1;
			f->STATE = ESTABLISHED;
		}
		return;
	}
	// SYN_SENT STATE
	if (f->STATE == SYN_SENT)
	{
		if ((flg & SYN) == SYN) // found SYN ACK
		{
			if ((flg & ACK) == ACK)
			{
				f->syn = seq;		
				f->STATE = ESTABLISHED;
			}
			else // WE RCVD A SYN ACK
			{
				f->STATE = SYN_RCVD;
			}
		}
		if ((flg & FIN) == FIN)
		{
			f->STATE = CLOSED;
		}
		return;
	}
	// SYN_RCVD
	if (f->STATE == SYN_RCVD)
	{
		if ((flg & ACK) == ACK)
		{
			f->STATE = ESTABLISHED;
		}
		if ((flg & FIN) == FIN)
		{
			f->STATE = CLOSED;
		}
		return;
	}
	
	// ESTABLISHED
	if (f->STATE == ESTABLISHED)
	{
		if ((flg & FIN) == FIN)
		{
			if (SENT)
			{
				if ((flg & ACK) == ACK)
				{
					f->STATE = CLOSING;
				}
				else
				{
					f->STATE = FIN_WAIT_1;
				}
			}
			else
			{
				f->STATE = CLOSE_WAIT;
			}
		}
		return;
	}
	
	// FIN_WAIT_1
	if (f->STATE == FIN_WAIT_1)
	{
		if ((flg & FIN) == FIN) // FIN
		{
			if (SENT == 0) // !SENT FIN
			{
				if ((flg & ACK) == ACK) // !SENT ACK FIN
				{
					f->STATE = TIME_WAIT;
				}
				else
				{
					f->STATE = CLOSING;
				}
			}
		}
		else if ((flg & ACK) == ACK) // !FIN ACK
		{
			if (SENT == 0) // !FIN !SENT ACK
			{
				f->STATE = FIN_WAIT_2;
			}
		}
		return;
	}
	
	// FIN_WAIT_2
	if (f->STATE == FIN_WAIT_2)
	{
		if ((flg & FIN) == FIN) // FIN
		{
			if (SENT == 0) // FIN !SENT
			{
				f->STATE = TIME_WAIT;
			}
		}
		return;
	}
	
	// CLOSING
	if (f->STATE == CLOSING)
	{
		if ((flg & ACK) == ACK) // ACK
		{
			if (SENT == 0) // ACK !SENT
			{
				f->STATE = TIME_WAIT;		
			}
		}
		return;
	}
	
	// CLOSE_WAIT
	if (f->STATE == CLOSE_WAIT)
	{
		if ((flg & FIN) == FIN)
			if (SENT)
				f->STATE = LAST_ACK;
		return;
	}
	
	// LAST_ACK
	if (f->STATE == LAST_ACK)
	{
		if ((flg & ACK) == ACK)
		{
			if (SENT == 0)
			{
				f->STATE = TIME_WAIT;
			}
		}
		return;
	}
}

void printAttributes(double * attributes, int size, char * end, char * identifier)
        {
        int i = 0;
	if (OUTPUT_ARFF == 1)
		{
	        for (i = 0; i < size; i++)
        	        {
                	fprintf(ARFF_OUTPUT,"%g,",*(attributes+i));
                	}
	        fprintf(ARFF_OUTPUT,"%s    %% %s\r\n",end,identifier);
		}
	fprintf(LOGFILE,"%s\n",identifier);
        }


int printIP(char * x, unsigned long ip)
	{
	return sprintf(x,"%s",inet_ntoa(ip));
	}


int classifyflow(struct flow *result)
{
	int * size = malloc(sizeof(int));
	double * attributes = calculateattributes(result->n,result->startpacket,size);
	char * identifier = malloc(sizeof(char) * 200);
	int x = 0;
	x += sprintf((identifier+x),"6/");
	x += printIP((identifier+x),result->n->srcip);
	x += sprintf((identifier+x),"/");
	x += sprintf((identifier+x),"%.5d/",result->n->srcport);
	x += printIP((identifier+x),result->n->dstip);
	x += sprintf((identifier+x),"/");
	x += sprintf((identifier+x),"%.5d/",result->n->dstport);
	x += sprintf((identifier+x),"%d.",result->n->starttime.tv_sec);
	x += sprintf((identifier+x),"%.6d",result->n->starttime.tv_usec);
	if (result->ABORTED == 1)
	{
		x += sprintf((identifier+x),"/ABORTED");
	}
	if (result->MID_STREAM_SAMPLE == 1)
	{
		x += sprintf((identifier+x),"/MIDSTREAM");
	}
	if (result->COMPLETE == 0)
	{
		x += sprintf((identifier+x),"/INCOMPLETE");
	}
	printAttributes(attributes,*size,"?",identifier);
	free(identifier);
	free(size);
	free(attributes);
	return 1; // we don't ever classify, so return -1
}


/**
 * Remove this flow object from the hashtable, mtflinkedlist and 
 * calculate attributes.
 */
int  removeflow(struct flow *result)	
{
	result->n->endtime = result->packet->time;
	int s = result->CLASS_VALUE;
	if (s == -1) 
		s = classifyflow(result);
	mtflinkedlist_remove(list,result->key);
	hashtable_remove(flows,result->key);	
	FLOWSCOMPLETED++;
	return s;
}

/**
 * Look at the tail of the linkedlist and see if the oldest flow has timed out
 * Remove this flow if this has occurred.
 */
int checktime()
{
	if (list->tail != NULL)
	{
		unsigned int xkey = list->tail->ELEMENT;
		struct flow *f = (struct flow *)hashtable_search(flows,xkey);
		if (f != NULL)
		{
			double d = elapsed(f->LASTPACKET, LASTPACKET);	
			if (d >= TIMEOUT * 1000000) // this is a minute
			{
			
				unsigned int packets = f->n->packets;
				unsigned int bytes = f->n->bytecount;
				unsigned int CLASS_VALUE = removeflow(f);	
				return CLASS_VALUE;	
			}
		}
	}	
	return -1;
}



/**
 * This is the main packet receiving method. Using the arguments, create new flows
 * and add packets to them. All of the demultiplexing happens in this method, so 
 * the functionality of the demuxer can be changed here. i.e. mid stream, data
 * reduction etc..
 */
void dealwithpacket(long sip, long dip, int spo, int dpo, int flg
			  , int ack, int seq
			  , unsigned int data, unsigned int lgt
			  , struct timeval packtime)
	{
	LASTPACKET = packtime; // save this packet as the last packet seen
	unsigned int key = createflowkey(sip,dip,spo,dpo);	
	// is this flow already in the hashtable
	struct flow *result = (struct flow *)hashtable_search(flows,key);

	/*
		IF THE RESULT IS NOT NULL BUT THIS IS A NEW SYN PACKET
		CHECK THAT ITS SEQUENCE NUMBER IS CORRECT AND THEN ESTABLISH
		A NEW FLOW: WE'VE PROBABLY MISSED THE RST PACKET

		ALSO, CHECK FOR RETRANS > 0, A RETRANSMITTED SYN == OLD FLOW
	 */
	if (result != NULL && ((flg & SYN) == SYN) && result->STATE > ESTABLISHED)
		{

		if (retrans(flg,data,ack,seq,result,result->n->srcip == sip ? 0 : 1,result->packet) == 0)
			{
			removeflow(result);
			result = NULL;
			}
		else
			{
			/**	
			THE NEW SYN WAS JUST A RETRANSMITTED PACKET FROM THE OLD FLOW
			CARRY ON AS USUAL AND ADD TO THE PACKET TRACE
			**/
			}
		}

	
	if (result == NULL)
	{
		struct flow *newf = createflow(seq,sip,dip,spo,dpo,key,lgt);
		settcpconnectionstate(sip,newf,flg,seq);

		if (newf->STATE == SYN_SENT || newf->STATE == SYN_RCVD || newf->STATE >= ESTABLISHED)
		{
			newf->n->starttime = packtime;
			addPacketToFlow(0, newf, packtime, lgt, data, flg,ack,seq);
			newf->startpacket = newf->packet;
			hashtable_insert(flows,key,newf);
       		        NEWFLOWS++;
		}
	}
	else
	{

		result->LASTPACKET = LASTPACKET;	

		/**
		 * If the -p or -d options are set, check if the flow has reached the
		 * right point. If so, remove it.
		 */
		if ((MAXIMUM_FLOW_LENGTH > 0 
			&& result->n->packets >= MAXIMUM_FLOW_LENGTH) 
		 	|| (MAXIMUM_FLOW_DURATION > 0 
			&& elapsed(result->n->starttime,LASTPACKET) 
						> MAXIMUM_FLOW_DURATION * 1000000))
		{
				
			/**
			 * here we remove the flow, if we wanted to implement
			 * a frequency based output then just classifyflow (attribs)
			 * based on the frequency.. don't remove the flow
			 */
			//removeflow(result);
		}
		else
		{
			// if it is then update the current flow information
			int sent = result->n->srcip == sip ? 0 : 1;

			addPacketToFlow(sent,result,packtime,lgt,data,flg,ack,seq);
			settcpconnectionstate(sip,result,flg,seq); // must be after add to packet

			// Is the flow now complete?
			if (result->STATE == TIME_WAIT)
			{
				result->COMPLETE = 1;
			}
		}
	}
	
	//  do a check to see if there are any timed out flows in our data structures
	// if there are, then remove them.
	int flow = -1;
	while ((flow = checktime()) != -1);
}

void streamfinished()
{
	LASTPACKET.tv_sec += TIMEOUT;
	int flow = -1;
	while ((flow = checktime()) != -1);
}

void setupdemuxer()
{
	
	/* Set up hashtable */
	flows = create_hashtable(20000,flowhashfn,flowequalfn, removeValue);
	//	printf("Hash Table Created\n");	
	/* Set up mtflinkedlist */
	list = mtflinkedlist_create();	
	int i = 0;

		
	LOGFILE = fopen("logfile","w");
	setupattributecalculator();	
	
}

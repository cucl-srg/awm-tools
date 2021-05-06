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
*   ANTc is distributed in the hope that it will be useful,                   *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with Trident; if not, write to the Free Software                    *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*                                                                             *
******************************************************************************/

#include "attributecalculator.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct attr_temp {

	// FEATURES GATHERED / USED IN LATER PROCESSING
	unsigned int packetsup;
	unsigned long bytesup;
	unsigned long databytesup;
	unsigned int packetsdown;
	unsigned long bytesdown;
	unsigned long databytesdown;
	unsigned int last_ack_server; // the largest ack seen
	unsigned int last_ack_client; // the largest ack seen
	unsigned int last_seq_server;
	unsigned int last_seq_client;
	unsigned int syn;
	unsigned int initial_done_server;
	unsigned int initial_done_client;
	//11 attributes
	unsigned int serverport;					// at start
	unsigned int push_packets_server;			// during capture
	unsigned int initial_window_bytes_server;   // during capture
	unsigned int initial_window_bytes_client;   // during capture
	double average_segment_size_server;			// during capture + / at end
	double average_segment_size_client;			// during capture + / at end
	double ip_data_bytes_median_server;   // at end
	double ip_data_bytes_median_client;   // at end
	unsigned int actual_data_packets_server;    // during capture
	unsigned int actual_data_packets_client;    // during capture
	double data_bytes_variance_server;			// at end
	double data_bytes_variance_client;			// at end
	unsigned int minimum_segment_size_server;   // during capture
	unsigned int minimum_segment_size_client;   // during capture
	unsigned int RTT_samples_client;
	unsigned int RTT_samples_server;
	unsigned int push_packets_client;           // during capture


} _attr_temp;



void setupattributecalculator()
{
if (OUTPUT_ARFF == 1)
	{
	fprintf(ARFF_OUTPUT,"% OUTPUT FROM PACKETGIVER ARFF FILE\r\n");
	fprintf(ARFF_OUTPUT,"@relation PACKETGIVER_ARFF\r\n");
	fprintf(ARFF_OUTPUT,"@attribute push_packets_server numeric\r\n");
	fprintf(ARFF_OUTPUT,"@attribute initial_window_bytes_server numeric\r\n");
	fprintf(ARFF_OUTPUT,"@attribute initial_window_bytes_client numeric\r\n");
	fprintf(ARFF_OUTPUT,"@attribute average_segment_size_server  numeric\r\n");
	fprintf(ARFF_OUTPUT,"@attribute ip_data_bytes_median_client numeric\r\n");
	fprintf(ARFF_OUTPUT,"@attribute actual_data_packets_client numeric\r\n");
	fprintf(ARFF_OUTPUT,"@attribute data_bytes_variance_server numeric\r\n");
	fprintf(ARFF_OUTPUT,"@attribute minimum_segment_size_client numeric\r\n");
	fprintf(ARFF_OUTPUT,"@attribute RTT_samples_client numeric\r\n");
	fprintf(ARFF_OUTPUT,"@attribute push_packets_client numeric\r\n");
	fprintf(ARFF_OUTPUT,"@attribute CLASS { ENTER YOUR CLASS NAMES HERE}\r\n");
	fprintf(ARFF_OUTPUT,"@data\n");
	}

}

int comp_nums(const int *num1, const int *num2)
{
	if (*num1 <  *num2) return -1;
	if (*num1 == *num2) return  0;
	if (*num1 >  *num2) return  1;
}

double rob_median(unsigned int arr[], int n)
{
	
	if (n == 1)
		return (double)arr[0];
	if (n == 0)
		return -1;
	
	qsort(
		  arr, 			/* Pointer to elements		*/
		  n, 			/* Number of elements		*/
		  sizeof(unsigned int),  			/* size of one element.		*/
		  (void *)comp_nums		/* Pointer to comparison function */
		  );
	if (n % 2 == 1)
		return (double)arr[n/2];
	else
		return (double)(arr[n/2] + arr[n/2-1])/2;
}

int findLastDataByte(unsigned int ack, struct packetlinkedlist *pkt, int SENT)
{
	unsigned int smallest_ack  = ack;
	struct packetlinkedlist *tp = pkt;
	for (tp = pkt; tp != NULL; tp = tp->packet)
	{
		if (SENT == tp->SENT)
			continue;
		if (tp->seq < smallest_ack)
			smallest_ack = tp->seq;
		unsigned int last_data_byte = tp->data + tp->seq - 1;
		if (tp->data == 0 || (tp->flags & FIN) == FIN) // increase by 1 if no data
			last_data_byte++;
		if (tp->RTT == 0 && last_data_byte + 1 == ack)
		{
			if ( tp->seq <= smallest_ack )
			{
			 	// now test that there are no other packets which have the same SEQ number as this found packet
				if (tp->retrans != 0)
					return 0;
				tp->RTT = 1;
				return 1;
			}
			else
				break;
		}
	}
	return 0;
}

void computePacketStats(int SENT, struct attr_temp *f, struct timeval packtime, unsigned int lgt, unsigned int data, int flg, unsigned int ack, unsigned int seq, struct packetlinkedlist *p)
{
	
	/**	CALCULATE IF THIS IS A VALID RTT SAMPLE **/
	
	int rtt = 0;
	if (p->retrans < 2)
	{	
		if (SENT && (flg & ACK) == ACK && ack > f->last_ack_server)
		{
			rtt = findLastDataByte(ack,p,SENT);
			f->RTT_samples_server += rtt; 
			if (p->retrans == 0 && ack > f->last_ack_server)
				f->last_ack_server = ack;
		}
		if (!SENT && (flg & ACK) == ACK && ack > f->last_ack_client)
		{
			rtt = findLastDataByte(ack,p,SENT);
			f->RTT_samples_client += rtt; 
			if (p->retrans == 0 && ack > f->last_ack_client)
				f->last_ack_client = ack;			
		}
	}
	/**	CALCULATE IF THIS IS A VALID RTT SAMPLE	**/	
	
	
	/**	DETERMINE INITIAL WINDOW BYTES **/
	/** CALCULATE WHETHER WE SHOULD CLOSE THE WINDOW OR NOT **/
	if (p->retrans == 0 && (flg & ACK) == ACK && f->packetsup + f->packetsdown > 3 && ack != f->syn+1)
	{
		if (f->initial_done_client == 0 && !SENT && f->databytesup > 0)
		{
			f->initial_done_client = ack;
			//printf("END OF INITIAL WINDOW\n");	
		}
		if (f->initial_done_server == 0 && SENT && f->databytesdown > 0)
		{
			f->initial_done_server = ack;	
			//printf("END OF INITIAL WINDOW\n");
		}
	}
	
	
	/** ADD ON THE DATA IF THE INITIAL WINDOW IS STILL OPEN **/
	if (p->retrans == 0 && (seq < f->initial_done_client || f->initial_done_client == 0) && SENT)
		f->initial_window_bytes_client += data;
	if (p->retrans == 0 && (seq < f->initial_done_server || f->initial_done_server == 0) && !SENT)
		f->initial_window_bytes_server += data;
	/**	DETERMINE INITIAL WINDOW BYTES **/
	
	
	/** UPDATE ALL THE OTHER ATTRIBUTES **/
	if (SENT)
	{
		f->packetsup++;
		f->bytesup+=lgt;
		f->databytesup+=data;
		if ((flg & PSH) == PSH)
			f->push_packets_client++;
		if (data > 0)
		{
			f->average_segment_size_client += data;
			f->actual_data_packets_client++;
			if (data < f->minimum_segment_size_client || f->minimum_segment_size_client == 0)
				f->minimum_segment_size_client = data;
		}
	}
	else
	{
		f->packetsdown++;
		f->bytesdown+=lgt;
		f->databytesdown+=data; 
		if ((flg & PSH) == PSH)
			f->push_packets_server++;
		if (data > 0)
		{
			f->average_segment_size_server += data;
			f->actual_data_packets_server++;
			if (data < f->minimum_segment_size_server || f->minimum_segment_size_server == 0)
				f->minimum_segment_size_server = data;
		}
	}
}


void replayPacketTrace(struct packetlinkedlist * startpacket, struct attr_temp * f)
{	
	struct packetlinkedlist *p = startpacket;
	if (p ==  NULL)
		return;
	
	for (; p != NULL; p = p->prev)
	{
		
		if (MAXIMUM_FLOW_LENGTH == -1 || MAXIMUM_FLOW_DURATION == -1)
		{
			if (f->initial_done_client == 1 && f->initial_done_server == 1)
				return;
		}
		
		if (MAXIMUM_FLOW_DURATION > 0)
		{
			if (elapsed(startpacket->time,p->time) > MAXIMUM_FLOW_DURATION * 1000000)
			{
		
				return;
			}
		}
		if (MAXIMUM_FLOW_LENGTH > 0)
		{
			if (f->packetsup + f->packetsdown >= MAXIMUM_FLOW_LENGTH)
			{
			
				return;
			}
		}
		computePacketStats(p->SENT,f,p->time,p->length,p->data,p->flags,p->ack,p->seq,p);
	}

}


double * calculateattributes(struct netflowrecord * details, struct packetlinkedlist * startpacket, int * size)
{
	
	struct attr_temp * result = malloc(sizeof(struct attr_temp));
	
	bzero(result,sizeof(struct attr_temp));
	result->syn = startpacket->seq;	


	replayPacketTrace(startpacket, result);	


	// CALCULATE REMAINING ATTRIBUTES
	
	struct packetlinkedlist *p = startpacket;	

	unsigned int * datac = malloc(result->packetsup * sizeof(unsigned int));

	unsigned int * datas = malloc(result->packetsdown * sizeof(unsigned int));


	;unsigned int i = 0;

	for (i = 0; i < result->packetsup; i++)
		*(datac+i) = 0;
	for (i = 0; i < result->packetsdown; i++)
		*(datas+i) = 0;

	i = 0;
	double avgup = (double)result->bytesup / (double)result->packetsup;
	double avgdown = (double)result->bytesdown / (double)result->packetsdown;
	double varc = 0, vars = 0;
	int j = 0;


	while (p != NULL)
	{
			if (p->SENT)
			{
				*(datac+i++) = p->length; // build up median array
				varc += (p->length - avgup)*(p->length - avgup);
			}
			else
			{
				*(datas+j++) = p->length; // build up median array
				vars += (p->length - avgdown)*(p->length - avgdown);
			}
		
		if (i + j > result->packetsup + result->packetsdown)
			break;
		
		p = p->prev;
	}	

	if (result->packetsup > 0)
	   	result->ip_data_bytes_median_client=rob_median(datac,result->packetsup);
	else	
		result->ip_data_bytes_median_client=0;
	
	if (result->packetsdown > 0)
		result->ip_data_bytes_median_server=rob_median(datas,result->packetsdown);
	else	
		result->ip_data_bytes_median_server=0;

		
	free(datac);

	free(datas);


	if (result->packetsdown > 1)
		result->data_bytes_variance_server = vars / (double)(result->packetsdown-1);
	else
		result->data_bytes_variance_server = 0;
	
	if (result->packetsup > 1)
		result->data_bytes_variance_client = varc / (double)(result->packetsup-1);
	else
		result->data_bytes_variance_client = 0;
	
	if (result->actual_data_packets_server > 0)
		result->average_segment_size_server /= (double)(result->actual_data_packets_server);
	else
		result->average_segment_size_server = 0;


	
	if (result->actual_data_packets_client > 0)
		result->average_segment_size_client /= (double)(result->actual_data_packets_client);
	else
		result->average_segment_size_client = 0;
	

	


	
	if (result->minimum_segment_size_server == -1)
		result->minimum_segment_size_server = 0;
	if (result->minimum_segment_size_client == -1)
		result->minimum_segment_size_client = 0;
	
	
	/** MAKE THE ATTRIBUTES ARRAY **/

	double * attributes = malloc(10 * sizeof(double));

	*(attributes) = result->push_packets_server;

	*(attributes+1) = result->initial_window_bytes_server;

	*(attributes+2) = result->initial_window_bytes_client;

	*(attributes+3) = result->average_segment_size_server;

	*(attributes+4) = result->ip_data_bytes_median_client;

	*(attributes+5) = result->actual_data_packets_client;

	*(attributes+6) = result->data_bytes_variance_server;

	*(attributes+7) = result->minimum_segment_size_client;

	*(attributes+8) = result->RTT_samples_client;

	*(attributes+9) = result->push_packets_client;		


	*size = 10;


	free(result);

	return attributes;
}

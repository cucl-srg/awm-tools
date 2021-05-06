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

#include "attributecalculator.h" // Don't really need an include here
#include "flowdemuxer.h"	 // Allows us to access the flow demultiplexer

/**
 * Various includes
 */
#include <pcap.h>
#include <time.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/**
 * PCAP variables (mostly taken from example online, so might not all be needed)
 */
bpf_u_int32 netmask = 0;	// Just need this
struct bpf_program fp;		// Compiled Filter expression from cmd line
char * filterExpression = "";	// Filter expression from cmd line
pcap_t *pcap1;			// handle to pcap thingy
char errbuf[PCAP_ERRBUF_SIZE];	// Need this as well (taken from example)
pcap_dumper_t *dc;		// Need this as well (taken fromeexample)
int pcap_dlt;			// Ditto

// TAKEN FROM TCPDPRIV
#if !defined(SLIP_HDRLEN)
#define SLIP_HDRLEN     16
#endif 
#define PPP_HDRLEN      4
#define NULL_HDRLEN     4
#define FDDI_HDRLEN     13

int dlt_hdrlen()
{
    switch (pcap_dlt) {
    case DLT_EN10MB: 
        return sizeof (struct ether_header);
    case DLT_SLIP:
        return SLIP_HDRLEN;
    case DLT_PPP:
        return PPP_HDRLEN;
#if     defined(FDDI_HDRLEN)
    case DLT_FDDI:
        return FDDI_HDRLEN;
#endif  /* defined(FDDI_HDRLEN) */
        case DLT_RAW:
                return 0;
    case DLT_NULL:
        return NULL_HDRLEN;
    default:
        fprintf(stderr, "unknown DLT %d\n", pcap_dlt);
        exit(1);
    }
}

/**
 * This is the callback method passed to the pcap library
 * 
 * For each packet we extract the relevant data:
 *	Source IP
 *	Destination IP
 *	Source Port
 *	Destination Port
 *	TCP Flags
 *	Acknowledgement Number
 *	Sequence Number
 * 	Packet Data Length
 *	Packet Total Length
 * 	Packet Time
 *
 *	Note:
 *		This is the only data collected as this is only what is needed
 *		for the original attributes (awmreduced.c). To allow any attribute
 *		generators to accept more attributes edit (flowdemuxer.h)		
 */
void dumper(u_char *user, const struct pcap_pkthdr *header1, const u_char *packet1)
{

	packet1 += dlt_hdrlen();
	struct iphdr *ip = (struct iphdr *)(packet1);
	packet1 += sizeof (*ip);
	if (ip->protocol != 6)
		{
		fprintf(stderr,"NON-TCP Packet - check filter expression?\n");
		return;
		}

	struct tcphdr *tcp = (struct tcphdr *)(packet1);
	long sip = ip->saddr;
	long dip = ip->daddr;  
	unsigned int dpo = ntohs(tcp->dest);
	unsigned int spo = ntohs(tcp->source);
	unsigned int flg = (tcp->fin*1+tcp->syn*2+tcp->rst*4+tcp->psh*8+tcp->ack*16);
	
	// get all the data we need from the packet
	unsigned int off = (tcp->doff);

	unsigned int lgt = ntohs(ip->tot_len);
	unsigned int data = lgt - 20 - (4 * off); //data bytes
	int ack = ntohl(tcp->ack_seq);
	int seq = ntohl(tcp->seq);


	if ((ntohs(ip->frag_off) & 0x3fff))
	{
		fprintf(LOGFILE,"IGNORING FRAGMENTED PACKET %u %u %u\n",
						ip->frag_off,spo,dpo);	
		return;
	}
	
	struct timeval time = header1->ts;

	dealwithpacket(sip,dip,spo,dpo,flg,ack,seq,data,lgt,time);
}


int readPackets(pcap_t *pcap1)
{
	pcap_dlt = pcap_datalink(pcap1);	
	
        if (pcap_compile(pcap1, &fp, filterExpression, 0, netmask) == -1)
	{
		fprintf(stderr,"Filter Did Not Compile\n");		
		exit(-1);
	}
	else
	{
		pcap_setfilter(pcap1, &fp);		
	}

	return pcap_loop(pcap1, 0, dumper, (u_char *)dc);
}

/**
 * Using the given file name, start capturing packets
 */
int getOfflineStream(char * ifile)
{
	printf("%s\n",ifile);
	pcap1 = pcap_open_offline(ifile,errbuf);
	return readPackets(pcap1);
}


/**
 * Using the given interface, starting capturing packets
 */
int getLiveStream(char * interface)
{
	pcap1 = pcap_open_live(interface, 80, 1, 1000, errbuf);
	bpf_u_int32 localnet;
	pcap_lookupnet(interface, &localnet, &netmask, errbuf);
	return readPackets(pcap1);
}

/**
 * Initialise Command Line Variables, Start the PCAP loop
 */
int main(int argc, char** argv)
{
	extern char *optarg;
	char * interface;
	char ch;
	int online = 0;

	while ((ch = getopt(argc,argv, "a:e:p:d:i:t:v")) != EOF)
	{
		switch (ch)
		{
		case 'a':
			OUTPUT_ARFF = 1;
			ARFF_OUTPUT = fopen(optarg,"w");
			break;
		case 'e':
			filterExpression = optarg;
			break;
		case 'p':
			MAXIMUM_FLOW_LENGTH = atoi(optarg);
			break;
		case 'd':
			MAXIMUM_FLOW_DURATION = atoi(optarg);
			break;
		case 'i':
			interface = optarg;
			online = 1;
			break;
		case 't':
			TIMEOUT = atoi(optarg);
			break;
		case 'v':
			ARFF_OUTPUT = stdout;
			OUTPUT_ARFF = 1;
			break;
		}
	}

	if (optind < argc)
		{
		printf("%i Input files after option list\n",argc-optind);
		online = 0;		
		}
	
	setupdemuxer();

	if (online)
	{
		printf("Opening Live Stream\n");
		getLiveStream(interface);		
	}
	else
	{
		while (optind < argc)
		{
		printf("Opening File Offline\n");
		getOfflineStream(argv[optind]);
		pcap_close(pcap1); 
		optind++;
		}
	}
	printf("Stream/Files Finished, Ending Incomplete Flows.\n");
	streamfinished(); // we are finished now, tell the demuxer
		
	return 1;
}

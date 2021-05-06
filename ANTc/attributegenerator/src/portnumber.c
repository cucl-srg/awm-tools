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

double * calculateattributes(struct netflowrecord * details,  // flow details
			struct packetlinkedlist * startpacket, // flow packets
			int * size) // number of attributes (we fill this in)
	{

	*size = 2;

	double * attributes = malloc(sizeof(double)*(*size));
	*attributes = details->srcport;
	*(attributes+1) = details->dstport;
	return attributes;
	
	}

void setupattributecalculator()
	{
	if (OUTPUT_ARFF == 1)
		{
		fprintf(ARFF_OUTPUT,"%% OUTPUT FROM ONLYPORTNUMBER ATTRIBUTES\r\n");
		fprintf(ARFF_OUTPUT,"@relation ONLY_PORT_NUMBER\r\n");
		fprintf(ARFF_OUTPUT,"@attribute SOURCE_PORT numeric\r\n");
		fprintf(ARFF_OUTPUT,"@attribute DESTINATION_PORT numeric\r\n");
		fprintf(ARFF_OUTPUT,"@attribute CLASS { ENTER YOUR CLASS NAMES HERE }\r\n");
		fprintf(ARFF_OUTPUT,"@data\r\n");
		}
	}

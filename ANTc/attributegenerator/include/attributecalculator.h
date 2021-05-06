#include "antctypes.h"
#include <stdio.h>

int MAXIMUM_FLOW_LENGTH = 0;
int MAXIMUM_FLOW_DURATION  = 0;

int OUTPUT_ARFF = 0;
FILE * ARFF_OUTPUT;



double * calculateattributes(struct netflowrecord * details, struct packetlinkedlist * startpacket, int * size);

void setupattributecalculator();

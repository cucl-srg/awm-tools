/******************************************************************************
*                                                                             *
*   Copyright 2006 Queen Mary University of London                            *
*   Copyright 1995 University of Cambridge                                    *
*                                                                             *
*   This file is part of stats.                                                *
*                                                                             *
*   stats is free software; you can redistribute it and/or modify              *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   stats is distributed in the hope that it will be useful,                   *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with Trident; if not, write to the Free Software                    *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*                                                                             *
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>
#include "sampleHistogram.h"

#include <unistd.h>
#include <string.h>



void usage(void);

int main( int argc,char ** argv)
{
    sampleHistogram history;
    FILE * file;
    sampleType low, hi, width, sample = 0,latex = 0;
    float l= 0,h =65536,w = 1;
    int i,done=0;
    int using1 = 0,one_col=1;
    float f=0.0;
    float cdf=0.0;
    int distribution = 0, all_distribution = 0,skip_first = 0;
    char buf[1000];
    int c,rc = 0;
    int errflg=0;
    int use_stdin=0;
int    normalise=0;
int    cumulate=0;
	int variance_output=0,variance_output_count=1;
    i=1;
    optarg=NULL;

#define ARGS "l:w:h:ncdsDL1V:"

    while(!errflg && ((c = getopt(argc, argv, ARGS)) != -1)) {

	switch(c) {      
	case 'V':
	    variance_output =  atoi( (optarg));
	    break;
      
	case 'c':
		cumulate++;
		normalise++;
	    break;
      
	case 'n':
		normalise++;
	    break;
      
	case 'l':
	    l =  atof( (optarg));
	    break;
      
	case 'w':
	    w =  atof( (optarg));
	    break;
      
	case 'h':
	    h =  atof( (optarg));
	    break;
      
	case 'd':
	    distribution = 1;
	    break;
      
	case 's':
	    skip_first = 1;
	    break;
      
	case 'D':
	    all_distribution = 1;
	    break;
      
	case 'L':
	    latex = 1;
	    break;
      
	case '1':
	    (one_col?(one_col = 0):(one_col = 1));
	    break;
      
	default:
	    errflg++;
	}
	i++;  
    }
    if(errflg) {
	usage();
    }




    if(!argv[i]) {
	use_stdin=1;
	i--;
    }else{
	use_stdin=0;
    }


	sprintf(buf,"\n%% file (%s)",(!use_stdin?argv[i]:"stdin"));

    while(i<argc) {

	if(!use_stdin) {
	    file = fopen(argv[i], "r");
	    if(! file) {  
		printf("cannot open file %s\n",argv[i]);
		exit(1);
	    }
	}else{
	    file=stdin;
	}
	fprintf(stderr,"#%s: using low=%g hi=%g width=%g file %s\n", 
		argv[0],l,h,w ,(!use_stdin?argv[i]:"stdin"));
	low =(sampleType)l;
	hi =(sampleType)h;
	width =(sampleType)w;
	bzero(&history,sizeof(sampleHistogram));
	if (!sampleHistogram_init(&history, low, hi, width))  {
	    printf("cannot initialise histogram  %g %g %g\n",l,h,w);
	    exit(1);
	}
	using1=one_col;
	if(skip_first) {
	    rc = fscanf(file, "%f", &f);
	}
	done=0;
	rc=0;
	while(rc != EOF ) {
	    if(using1) {    
		rc = fscanf(file, "%f", &f);
		if(rc != EOF /* && f > 0 */) {	
		    if(f <= hi)
			sampleHistogram_add(&history, (sampleType) f);
		    else
			fprintf(stderr,"dropped sample %f\n",f);
		}
	    }
	    else {
		rc = fscanf(file, "%f %f\n", &f,(float *) &sample);
		if(rc != 2 && rc != EOF) {
		    printf("using only 1 column\n");
		    using1 = 1;
		    continue;
		}
		if(rc == 2 && f >= 0) {
		    if(sample > 0)
			sampleHistogram_addObsToSample(&history, (sampleType) f, sample);
		}
	    }
	    done++;
	    if(variance_output && variance_output_count >= variance_output 
			&& rc != EOF) {
		    sampleStatistic_line(stdout, & history.stats);
			variance_output_count=1;
        }else{
			variance_output_count++;
		}
	}
	if(done) {
	    sampleHistogram_aggregate( & history); 
	    sampleStatistic_dump(stdout, & history.stats,"results");
	    if(distribution) {
		int i;
		if(all_distribution) {
		    for(i = 0; i < history.howManyBuckets; i++) {	
			printf("%f %g\n",(float)
			       (sampleHistogram_bucketThreshold(&history, i) - width),
(normalise?history.bucketCount[i]/history.stats.n:history.bucketCount[i]));
		    }
		} else {
		    for(i = 0; i < history.howManyBuckets; i++) {	
			if(history.bucketCount[i] > 0) {
			    printf("%f %g ",(float)
				   (sampleHistogram_bucketThreshold(&history, i) - width),
(normalise?history.bucketCount[i]/history.stats.n:history.bucketCount[i]));
                            if(cumulate) {
                                     cdf += history.bucketCount[i]/history.stats.n;
                                     printf("%g\n",(float)cdf);
                            }else{
                                     printf("\n");
                            }
                            }

		    }
	  
		}
	    }
	    if(latex) {      
		sprintf(buf,"\n%% file (%s)",(!use_stdin?argv[i]:"stdin"));
		sampleStatistic_latex(stderr, & history.stats, buf,0.95);
	    }
	    else
		printf("# samples: %d\n", (int)history.stats.n);
	}
	else 
	    printf("no samples read\n");
	i++;
    }
    exit(0);
}

void 
usage()
{
    printf("%s: -{options} <files>\n",__FUNCTION__ );
    printf(" -l = low\n");
    printf(" -w = width\n");
    printf(" -s = first value is length (i.e. skip first line)\n");
    printf(" -h = high\n");
    printf(" -n = normalise\n");
    printf(" -c = cdf (includes normalisation)\n");
    printf(" -d = distribution\n");
    printf(" -D = distribution all values\n");
    printf(" -L = LaTeX output to stderr\n");
    printf(" -1 = 1 column file\n");
    printf(" -V# = produce a single line output every # lines\n");
    exit(1);
}

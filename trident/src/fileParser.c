/******************************************************************************
*                                                                             *
*   Copyright 2006 Queen Mary University of London                            *
*                                                                             *
*   This file is part of Trident.                                             *
*                                                                             *
*   Trident is free software; you can redistribute it and/or modify           *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   Trident is distributed in the hope that it will be useful, but            *
*   WITHOUT ANY WARRANTY; without even the implied warranty of                *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with Trident; if not, write to the Free Software                    *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*                                                                             *
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define	MAX_SIZE (16384)
#define	DISTR_ERR (1.0e-5)	// max. inconsistency of the given distribution

int fileParser(int argc, char *argv[]){
	
	static	char	nonasc_err[] = "Non-ascending X-values encountered\n";
	static	char	prob_err[] = "Invalid probability encountered\n";
	static	char	read_err[] = "Error reading file\n";
	static	char	consist_err[] = "Distribution inconsistent\n";
	
	double table [MAX_SIZE];	
	FILE *in_file;
	char *ptr; 
	char line [101]; 
	int no_values, pos; 
	double x, xold,sum, prob;
	double Pdf, z;
	
	if(argc != 3){
		printf("Usage: %s filename\n", argv[0]);
		exit(0);
	} 
	in_file = fopen(argv[2], "r"); 	/*Open file input*/
	if (in_file == NULL){
		fprintf(stderr, "Error: Unable to open file %s\n", argv[2] );
		exit(8);
	}
	else{
		sum = 0.0;
		xold = 0.0;
		//fprintf(stderr,"%s(%d): before while\n",__FUNCTION__,__LINE__); // Debugging
		while(((ptr = fgets(line, sizeof(line), in_file))!=NULL) && (no_values = sscanf(line,"%lg %lg",&x, &prob)) == 2){ 
		              	if(x <= xold) {
					printf(nonasc_err);
					exit(-1);
				}
				xold = x;
				if (prob < 0.0 || prob > 1.0) {
					printf(prob_err);
					exit(-1);
										
				}
				sum = sum + prob;			
		}
		if ((ptr !=NULL)&&(no_values != 2)){
		  fprintf(stdout, "Error reading line %s\n", line);
		  exit(-1);
		}
		rewind(in_file);
		if (fabs(1.0 - sum) > DISTR_ERR){
		  printf(consist_err);
		  exit(-1);
		}
		
		Pdf = 0.0;
		x = 0.0;
		for (pos = 0; pos < MAX_SIZE ; ++pos){
			z = (pos + 0.5) / (double) MAX_SIZE;	
			while (Pdf < z){	/* look for the smallest X, for which the distribution function is not smaller than z */
				if ((no_values = fscanf(in_file, "%lg %lg", &x, &prob)== 2)){
				  if ((ptr !=NULL)&&(no_values !=2)){
					  fprintf(stdout, read_err);
					  exit(-1);
				  }
				}
				Pdf += prob / sum;
			
			}
			table[pos] = x;
		}
//		|VALUE|DISTRIBUTION|
		for (pos = 0; pos < MAX_SIZE; ++pos) {
			fprintf(stdout,"%lg %lg\n",table[pos], (double)(pos/(double) MAX_SIZE));

		}
	}
	int status;
	status = fclose(in_file);
	if (status){
		fprintf(stderr, "%s did not close\n", argv[2]);
		exit(-1);
	}

/* Random Generator to access the table */
	{
	  int i;
	  double rv;
	  
	  for (i=0; i < 10000000; ++i) {
	    /* pos must be >= 0 and < MAX_SIZE */
	    pos=(int) (MAX_SIZE * (lrand48() / ( pow(2,31)  + 1.0))); 
	    rv=table[pos];
// |INDEX|TABLE POSITION|RETURN VALUE|:
//	    fprintf(out_file," %d  %d  %g\n",i,pos,rv);
	  }
	}
	exit(0);	
}

/******************************************************************************
*                                                                             *
*   Copyright 2006 Queen Mary University of London                            *
*   Copyright 1993 University of Cambridge                                    *
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

/* 
 * $Id: sampleHistogram.c,v 1.1 1997/11/11 12:51:02 tjg21 Exp awm22 $
 *
 * 
 *
 * modified by: Simon Crosby, University of Cambridge
 * Date      : Sun Dec 19 11:00:02 GMT 1993
 */



/* Copyright (C) 1988 Free Software Foundation
 * written by Dirk Grunwald (grunwald@cs.uiuc.edu)
 * 
 *      This file is part of GNU CC.
 * 
 *      GNU CC is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY.  No author or distributor
 *      accepts responsibility to anyone for the consequences of using it
 *      or for whether it serves any particular purpose or works at all,
 *      unless he says so in writing.  Refer to the GNU CC General Public
 *      License for full details.
 * 
 *      Everyone is granted permission to copy, modify and redistribute
 *      GNU CC, but only under the conditions described in the
 *      GNU CC General Public License.   A copy of this license is
 *      supposed to have been given to you along with GNU CC so you
 *      can know your rights and responsibilities.  It should be in a
 *      file named COPYING.  Among other things, the copyright notice
 *      and this notice must be preserved on all copies.  
 */

#ifdef wanda
#undef wanda
#endif

#if !defined (wanda)
#include <math.h>
#if defined(linux)
#include <values.h>
#endif
#else
#if defined (KERNEL)
#define malloc Malloc
#define free Free
#endif
#endif


#ifdef  sampleHistogram_TRACE
#define TRACE(x)  x
#else
#define TRACE(x)
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "sampleHistogram.h"

const int sampleHistogramMinimum = -2;
const int sampleHistogramMaximum = -1;


/* ---------------------------------------------------------------------- */

int
sampleHistogram_init(sampleHistogram *s, sampleType low, 
		     sampleType high, sampleType width)
{
  sampleType t = high;
  int i;
  if(! s) {  
    return 0;
  }
  sampleStatistic_reset(& (s->stats));
  if (high < low) {
    high = low;
    low = t;
  }
  s->min = low;
  s->max = high;
  if (width == -1) {
    width = (high - low) / 10;
  }
  if (s->bucketCount && s->howManyBuckets) free((char *)s->bucketCount);
  s->bucketWidth = width;

  s->howManyBuckets =  (int)((high - low) / width) + 3;
  s->bucketCount = (sampleType *)malloc(s->howManyBuckets *
					sizeof(sampleType));
  if(!  (s->bucketCount )) {  
    TRACE (printf("sampleHistogram_init: failed\n"));
    return 0;
  }
  for (i = 0; i < s->howManyBuckets; i++) {
    s->bucketCount[i] = 0;
  }
  TRACE (printf("sampleHistogram_init: %#x %d %d %d\n",s,(int)low,
		(int)high,(int)width));
  return 1;
}

/* ---------------------------------------------------------------------- */

sampleHistogram *
sampleHistogram_create(sampleType low, sampleType high, sampleType width)
{
  sampleHistogram *s  = (sampleHistogram *)malloc(sizeof(sampleHistogram));
  if(! s) {  
    return NULL;
  }
  if(!sampleHistogram_init(s, low, high, width)) {  
    free((char *)s);
    return NULL;
  }
  return s;
}

void
sampleHistogram_destroy(sampleHistogram *s)
{
  if (s->bucketCount) {
    free ((char *)s->bucketCount);
  }
  free((char *)s);
}

/* ---------------------------------------------------------------------- */

/* This is used only when elements have been added w/out doing sample stat
 * aggregation on the fly - eg in assembler code. Relies on width == 1
 */

int
sampleHistogram_aggregate(sampleHistogram *s)
{
  int i;
  sampleType value = s->min;
  sampleType tmp;
  if(s->bucketWidth != 1) return 0;
  sampleStatistic_reset(& (s->stats));
  for (i = 0; i < s->howManyBuckets; i++) {
    if(s->bucketCount[i]) {    
      tmp = s->bucketCount[i] * value;
      s->stats.n += s->bucketCount[i];
      s->stats.x += tmp;
      s->stats.x2 += tmp * value;
      if ( s->stats.minValue > value) s->stats.minValue = value;
      if ( s->stats.maxValue < value) s->stats.maxValue = value;
      
    }
    value ++;
  }
  return 1;
}


/* ---------------------------------------------------------------------- */

void
sampleHistogram_add(sampleHistogram *s, sampleType value)
{
#ifdef ITER  
   int i;
  sampleType compare = s->min;
  sampleStatistic_add(& (s->stats),value);
  for (i = 0; i < s->howManyBuckets; i++) {
    if (value <= compare) break;
    compare += s->bucketWidth;
  }
  if(i == s->howManyBuckets) {    
    i = s->howManyBuckets-1;		/* may not have value of huge */
  }
  TRACE (printf("add %E to %d limit %E\n",value,i,compare));
  s->bucketCount[i]++;
#else 
  int compare;
  sampleStatistic_add(& (s->stats),value);
  if(value < s->min) {
    s->bucketCount[0]++;
    TRACE (printf("add %E to %d limit %E\n",value,0,s->min));
    return;
  }
  compare = (int)
    (( value - s->min ) / s->bucketWidth);
  if(value > (compare * s->bucketWidth + s->min))
    compare++;
  if(compare >= s->howManyBuckets)
    compare = s->howManyBuckets-1;
  TRACE (printf("add %E to %d limit %E\n",value,compare,
	 (compare * s->bucketWidth + s->min)));
  s->bucketCount[compare]++;
#endif    
}

/* ---------------------------------------------------------------------- */

void
sampleHistogram_addObsToSample(sampleHistogram *s, sampleType value, 
			       sampleType obs2add)
{
#ifdef ITER  
   int i;
  sampleType compare = s->min;
  sampleStatistic_add(& (s->stats),value);
  for (i = 0; i < s->howManyBuckets; i++) {
    if (value <= compare) break;
    compare += s->bucketWidth;
  }
  if(i == s->howManyBuckets) {    
    i = s->howManyBuckets-1;		/* may not have value of huge */
  }
  TRACE (printf("add %E to %d limit %E\n",obs2add,i,compare));
  s->bucketCount[i] += obs2add;
#else 
  int compare;
  sampleStatistic_add(& (s->stats),value);
  if(value < s->min) {
    s->bucketCount[0] += obs2add;
    TRACE (printf("add %E to %d limit %E\n",obs2add,0,s->min));
    return;
  }
  compare = (int)
    (( value - s->min ) / s->bucketWidth);
  if(value > (compare * s->bucketWidth + s->min))
    compare++;
  if(compare >= s->howManyBuckets)
    compare = s->howManyBuckets-1;
  TRACE (printf("add %E to %d limit %E\n",obs2add,compare,
	 (compare * s->bucketWidth + s->min)));
  s->bucketCount[compare] += obs2add;
#endif    
}

/* ---------------------------------------------------------------------- */

sampleType
sampleHistogram_similarSamples(sampleHistogram *s, sampleType d)
{
  sampleType compare = s->min;
  int i;
  for (i = 0; i < s->howManyBuckets; i++) {
    compare += s->bucketWidth;
    if (d <= compare) return(s->bucketCount[i]);
  }
  return(0);
}

/* ---------------------------------------------------------------------- */

void
sampleHistogram_reset(sampleHistogram *s)
{
  int i;
  sampleStatistic_reset(& (s->stats));
  if (s->howManyBuckets > 0 && s->bucketCount != NULL) {
    for (i = 0; i < s->howManyBuckets; i++) {
      s->bucketCount[i] = 0;
    }
  }
}

/* ---------------------------------------------------------------------- */

#if !defined (__STDC__)
/* these will be inline otherwise */

int
sampleHistogram_buckets(sampleHistogram *s)
{
  return( s->howManyBuckets );
}

sampleType
sampleHistogram_bucketThreshold(sampleHistogram *s, int i)
{
  sampleType compare = s->min + (i+1) * s->bucketWidth;
  if(compare > s->max)
    return  0;
  return(compare);
}

sampleType
sampleHistogram_inBucket(sampleHistogram *s, int i)
{
  if (i < 0 || i >= s->howManyBuckets)
    return 0;
  return(s->bucketCount[i]);
}

#endif

/* ---------------------------------------------------------------------- */

#if !defined (wanda)
void
sampleHistogram_dump(FILE *f, sampleHistogram *s, char * title)
{
  int i;
  sampleType compare = s->min;
  sampleStatistic_dump(f, & s->stats, title);
  fprintf(f, "#%s: buckets %d low: %E high: %E width: %E\n", title,
	  s->howManyBuckets,
	  s->min,s->max,s->bucketWidth);
  if(s->bucketCount[0])
    fprintf(f,"%E, %E\n", s->min, s->bucketCount[0]);
  for(i = 1; i < s->howManyBuckets-1; i++) {  
    compare += s->bucketWidth;
    if(s->bucketCount[i])
      fprintf(f,"%E, %E\n", compare, s->bucketCount[i]);
  }
  if(s->bucketCount[s->howManyBuckets-1])
    fprintf(f,"%E, %E\n", maxSample, s->bucketCount[s->howManyBuckets-1]);
}
#else

#if defined (KERNEL)
void
sampleHistogram_dump(sampleHistogram *s, char * title)
{
  int i;
  sampleType compare = s->min;
  sampleStatistic_dump(& s->stats, title);
  printf("#%s: buckets %d low: %d high: %d width: %d\n", title,
	 s->howManyBuckets,
	  s->min,s->max,s->bucketWidth);
  if(s->bucketCount[0])
    printf("%d, %d\n", s->min,	   s->bucketCount[0]);
  for(i = 1; i < s->howManyBuckets-1; i++) {  
    compare += s->bucketWidth;
    if(s->bucketCount[i])
      printf("%d, %d\n",compare,s->bucketCount[i]);
  }
  if(s->bucketCount[s->howManyBuckets-1])
    printf("%d, %d\n", maxSample, s->bucketCount[s->howManyBuckets-1]);
}
#else
void
sampleHistogram_dump(FILE *f, sampleHistogram *s, char * title)
{
  int i;
  sampleType compare = s->min;
  sampleStatistic_dump(f, & s->stats, title);
  fprintf(f, "#%s: buckets %d low: %d high: %d width: %d\n", title,
	  s->howManyBuckets,
	  s->min,s->max,s->bucketWidth);
  if(s->bucketCount[0])
    fprintf(f,"%d, %d\n", s->min,
	   s->bucketCount[0]);
  for(i = 1; i < s->howManyBuckets-1; i++) {  
    compare += s->bucketWidth;
    if(s->bucketCount[i])
      fprintf(f,"%d, %d\n",  compare,
	      s->bucketCount[i]);
  }
  if(s->bucketCount[s->howManyBuckets-1])
    fprintf(f,"%d, %d\n", maxSample,
	   s->bucketCount[s->howManyBuckets-1]);
}
#endif
#endif

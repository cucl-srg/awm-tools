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
 * $Id: sampleHistogram.h,v 1.1 1997/11/11 12:51:30 tjg21 Exp $
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


#ifndef sampleHistogram_h
#define sampleHistogram_h 

#include "sampleStatistic.h"

#ifndef ASSEMBLER

extern const int sampleHistogramMinimum;
extern const int sampleHistogramMaximum;


/* NOTE: do not change this structure as its offsets are hard coded
 *	 for use by assembler code
 */

typedef struct _sampleHistogram {
  sampleType 		*bucketCount;
  int 			howManyBuckets;
  sampleType 		min, max;
  sampleType 		bucketWidth;
  sampleStatistic 	stats;		/* 5 words */
} sampleHistogram;

#else /* assembler */

#define S_HIST_BUCKETS		0
#define S_HIST_NBUCK		4
#define S_HIST_MIN		8
#define S_HIST_MAX		12
#define S_HIST_BWID		16
#define S_HIST_STATS		20
#define S_HIST_SIZE		(20 + S_STAT_SIZE)

#endif

#ifndef ASSEMBLER

#ifdef __STDC__
#define _(x)	x
#else
#define _(x)	()
#endif

extern sampleHistogram *sampleHistogram_create _((sampleType low, 
						  sampleType high,
						  sampleType width));
extern int sampleHistogram_init  _((sampleHistogram *s,
				     sampleType low, 
				     sampleType high,
				     sampleType width));
extern void sampleHistogram_destroy _((sampleHistogram *s));
extern void sampleHistogram_add _((sampleHistogram *s, sampleType value));
extern sampleType sampleHistogram_similarSamples _((sampleHistogram *s,
					     sampleType d));
extern void sampleHistogram_reset _((sampleHistogram *s));

extern int sampleHistogram_aggregate _((sampleHistogram *s));
extern void sampleHistogram_addObsToSample(sampleHistogram *s, 
					   sampleType sample, 
					   sampleType obs2add);

#if !defined (KERNEL)
extern void sampleHistogram_dump _((FILE *f, sampleHistogram *s, char * title));
#else
extern void sampleHistogram_dump _((sampleHistogram *s, char * title));
#endif

#undef _

#ifdef __STDC__

static inline int
sampleHistogram_buckets(sampleHistogram *s)
{
  return( s->howManyBuckets );
}

static inline sampleType
sampleHistogram_bucketThreshold(sampleHistogram *s, int i)
{
  sampleType compare = s->min + (i+1) * s->bucketWidth;
  if(compare > s->max)
    return  0;
  return(compare);
}

static inline sampleType
sampleHistogram_inBucket(sampleHistogram *s, int i)
{
  if (i < 0 || i >= s->howManyBuckets)
    return 0;
  return(s->bucketCount[i]);
}


#else

extern int sampleHistogram_buckets	_((sampleHistogram *s));
extern sampleType sampleHistogram_bucketThreshold	_((sampleHistogram *s, int i));
extern sampleType sampleHistogram_inBucket _((sampleHistogram *s, int i));

#endif

#endif /* ASSEMBLER */
#endif

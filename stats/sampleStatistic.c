
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
 * $Id: sampleStatistic.c,v 1.2 1999/03/30 10:24:56 awm22 Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include "sampleStatistic.h"

sampleStatistic *
sampleStatistic_create()
{
  sampleStatistic *s = malloc(sizeof(sampleStatistic));
  if(! s) {  
    return NULL;
  }
  s->n = 0; s->x = s->x2 = 0;
  s->maxValue = -maxSample;
  s->minValue = maxSample;
  return s;
}

void
sampleStatistic_destroy(sampleStatistic *s)
{
  free(s);
}

void
sampleStatistic_reset(sampleStatistic *s)
{
  s->n = 0; s->x = s->x2 = 0;
  s->maxValue = minSample;
  s->minValue = maxSample;
}

void
sampleStatistic_add(sampleStatistic *s, sampleType value)
{
  s->n++;
  s->x += value;
  s->x2 += (value * value);
  if ( s->minValue > value) s->minValue = value;
  if ( s->maxValue < value) s->maxValue = value;
}

sampleType
sampleStatistic_mean(sampleStatistic *s)
{
  if ( s->n > 0) {
    return (s->x / s->n);
  }
  else {
    return ( 0 );
  }
}

sampleType
sampleStatistic_samples(sampleStatistic *s)
{
  return(s->n);
}

sampleType
sampleStatistic_min(sampleStatistic *s)
{
  return(s->minValue);
}

sampleType
sampleStatistic_max(sampleStatistic *s)
{
  return(s->maxValue);
}

sampleType
sampleStatistic_sum(sampleStatistic *s)
{
  return(s->x);
}


#if !defined(wanda)
/* 
 * on most systems this will work, however on systems without
 * floating point arithmetic, the remainder of the functions are ridiculous
 */


sampleType
sampleStatistic_var(sampleStatistic *s)
{
  if ( s->n > 1) {
    return(( s->x2 - ((s->x * s->x) /  s->n)) / ( s->n - 1));
  }
  else {
    return ( 0 );
  }
}

sampleType
sampleStatistic_stdDev(sampleStatistic *s)
{
  if ( s->n <= 0 || sampleStatistic_var(s) <= 0) {
    return(0);
  } else {
    return( (sampleType) sqrt( sampleStatistic_var(s) ) );
  }
}

/* t-distribution: given p-value and degrees of freedom, return t-value
 * adapted from Peizer & Pratt JASA, vol63, p1416
 */

sampleType
sampleStatistic_tval(sampleType p, int df) 
{
  sampleType t;
  sampleType ddf =0;
  sampleType a =0;
  sampleType aa = 0;
  int positive = p >= 0.5;
  
  p = (positive)? 1.0 - p : p;
  if (p <= 0.0 || df == 0)
    t = HUGE;
  else if (p == 0.5)
    t = 0.0;
  else if (df == 1)
    t = 1.0 / tan((p + p) * 1.57079633);
  else if (df == 2)
    t = sqrt(1.0 / ((p + p) * (1.0 - p)) - 2.0);
  else {	
    ddf = (sampleType)df;
    a = sqrt(log(1.0 / (p * p)));
    aa = a * a;
    a = a - ((2.515517 + (0.802853 * a) + (0.010328 * aa)) /
	     (1.0 + (1.432788 * a) + (0.189269 * aa) +
	      (0.001308 * aa * a)));
    t = ddf - 0.666666667 + 1.0 / (10.0 * ddf);
    t = sqrt(ddf * (exp(a * a * (ddf - 0.833333333) / (t * t)) - 1.0));
  }
  return (positive)? t : -t;
}

sampleType
sampleStatistic_confidencei(sampleStatistic *s,int interval)
{
  sampleType samp = sampleStatistic_samples(s) - 1;
  sampleType t;
  if(samp <= 0)
    return(0);
  t = sampleStatistic_tval(((sampleType)(100 + interval)) * 
			   0.005, (int)samp );
  if (t == HUGE)
    return t;
  else
    return (t * sampleStatistic_stdDev(s)) / sqrt(samp);
}

sampleType
sampleStatistic_confidenced(sampleStatistic *s,sampleType p_value)
{
  sampleType samp = sampleStatistic_samples(s) - 1;
  sampleType t;
  if(samp <= 0)
    return(0);
  t = sampleStatistic_tval((1.0 + p_value) * 0.5, (int)samp);
  
  if (t == HUGE)
    return t;
  else
    return (t * sampleStatistic_stdDev(s)) / sqrt(samp);
}


void
sampleStatistic_dump(FILE *f, sampleStatistic *s, char *title)
{
  fprintf(f,"#%s: samples %E  minVal %E\n", title,
	    sampleStatistic_samples(s),
	    sampleStatistic_min(s));
  fprintf(f,"#%s: maxVal  %E  sum %E\n", title,
	    sampleStatistic_max(s),
	    sampleStatistic_sum(s));
  fprintf(f,"#%s: mean    %E  var    %E  stdDev %E\n", title,
	    sampleStatistic_mean(s),
	    sampleStatistic_var(s),
	    sampleStatistic_stdDev(s));
  fprintf(f,"#%s: conf90  %E  conf95 %E  conf99 %E\n", title,
	    sampleStatistic_confidenced(s,(sampleType)0.9),
	    sampleStatistic_confidenced(s,(sampleType)0.95),
	    sampleStatistic_confidenced(s,(sampleType)0.99));
}

void
sampleStatistic_latex(FILE *f, sampleStatistic *s, char *title, 
		      sampleType CI)
{

  fprintf(f,"%% Mean & Var & Std. Dev. & %3.0f \\%% CI \\\\ \\hline %% %s\n",
	  CI * 100, title);
  fprintf(f,"%E\t& %E\t& %E\t& %E \\\\\n",
	  sampleStatistic_mean(s),
	  sampleStatistic_var(s),
	  sampleStatistic_stdDev(s),
	  sampleStatistic_confidenced(s,CI));
}

void
sampleStatistic_line(FILE *f, sampleStatistic *s)
{

  fprintf(f,"%E %E %E %E %E %E %E %E %E %E\n",
	    sampleStatistic_samples(s),
	    sampleStatistic_min(s),
	    sampleStatistic_max(s),
	    sampleStatistic_sum(s),
	  sampleStatistic_mean(s),
	  sampleStatistic_var(s),
	  sampleStatistic_stdDev(s),
	    sampleStatistic_confidenced(s,(sampleType)0.9),
	    sampleStatistic_confidenced(s,(sampleType)0.95),
	    sampleStatistic_confidenced(s,(sampleType)0.99));
}

#endif

#if defined (wanda)
#if defined (KERNEL)
void
sampleStatistic_dump(sampleStatistic *s, char *title)
{
  printf("#%s: samples %d  minVal %d\n", title,
	    sampleStatistic_samples(s),
	    sampleStatistic_min(s));
  printf("#%s: sum x  %d  sum x^2 %d\n", title,
	    (s->x),
	    (s->x2));
  printf("#%s: maxVal  %d  sum %d (int)mean %d\n", title,
	    sampleStatistic_max(s),
	    sampleStatistic_sum(s),
	    sampleStatistic_mean(s));
}
#else
void
sampleStatistic_dump(FILE *f, sampleStatistic *s, char *title)
{
  fprintf(f,"#%s: samples %d  minVal %d\n", title,
	    sampleStatistic_samples(s),
	    sampleStatistic_min(s));
  fprintf(f,"#%s: sum x  %d  sum x^2 %d\n", title,
	    (s->x),
	    (s->x2));
  fprintf(f,"#%s: maxVal  %d  sum %d (int)mean %d\n", title,
	    sampleStatistic_max(s),
	    sampleStatistic_sum(s),
	    sampleStatistic_mean(s));
}
#endif
#endif

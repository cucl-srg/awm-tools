
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
 * $Id: sampleStatistic.h,v 1.2 1999/03/30 10:24:56 awm22 Exp $
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


#ifndef sampleStatistic_h
#define sampleStatistic_h 

#ifndef ASSEMBLER

#include <stdio.h>


#if defined (wanda)
typedef int 		sampleType;
#define maxSample	(0x7fffffff)
#define minSample	-(maxSample)
#else
typedef double 		sampleType;
#define maxSample	MAXFLOAT
#define minSample	-MAXFLOAT
#endif

/* NOTE: do not change this structure as its offsets are hard coded
 *	 for use by assembler code
 */


typedef struct {
    sampleType n;
    sampleType x;
    sampleType x2;
    sampleType minValue, maxValue;
}  sampleStatistic;

#else /* assembler */

#define S_STAT_N		0
#define S_STAT_X		4
#define S_STAT_X2		8
#define S_STAT_MIN		12
#define S_STAT_MAX		16
#define S_STAT_SIZE		20

#endif

#ifndef ASSEMBLER

#ifdef __STDC__
#define _(x)	x
#else
#define _(x)	()
#endif

extern sampleStatistic *sampleStatistic_create _((void));
extern void   sampleStatistic_destroy _((sampleStatistic *s));
extern void   sampleStatistic_reset _((sampleStatistic *s));

#if !defined (wanda)
extern sampleType sampleStatistic_confidenced _((sampleStatistic *s,
						 sampleType p_value));
extern sampleType sampleStatistic_confidencei _((sampleStatistic *s,
						 int interval));
extern sampleType sampleStatistic_tval _((sampleType p, int df));
extern sampleType sampleStatistic_stdDev _((sampleStatistic *s));
extern sampleType sampleStatistic_var _((sampleStatistic *s));
#endif

extern sampleType sampleStatistic_max _((sampleStatistic *s));
extern sampleType sampleStatistic_min _((sampleStatistic *s));
extern sampleType sampleStatistic_samples _((sampleStatistic *s));
extern sampleType sampleStatistic_sum _((sampleStatistic *s));
extern sampleType sampleStatistic_mean _((sampleStatistic *s));
extern void sampleStatistic_add _((sampleStatistic *s,sampleType value));

#if !defined (KERNEL)
extern void sampleStatistic_dump _((FILE *f, sampleStatistic *s,
				    char * title));
extern void
sampleStatistic_line _((FILE *f, sampleStatistic *s));
extern void
sampleStatistic_latex _((FILE *f, sampleStatistic *s, char *title, 
		      sampleType CI));
#else
extern void sampleStatistic_dump _((sampleStatistic *s, char * title));
#endif

#undef _

#endif/* assembler */

#endif

/******************************************************************************
*                                                                             *
*   Copyright 2006 Queen Mary University of London                            *
*   Copyright 2005 University of Cambridge Computer Laboratory.               *
*                                                                             *
*                                                                             *
*   This file is part of Trident.                                             *
*                                                                             *
*   Trident is free software; you can redistribute it and/or modify           *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   Trident is distributed in the hope that it will be useful,                 *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with Trident; if not, write to the Free Software                    *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*                                                                             *
******************************************************************************/


#ifndef _trident_util_h_
#define _trident_util_h_

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define ffs_long     ffs /* this will only work on 32 bit machines! */

void          fatal(char *, ...);
void          warning(char *, ...);
int           getint(char*);
double        getdouble(char*);
unsigned long host2ipaddr(const char*);
int           tvless(struct timeval*, struct timeval*);
int           tveqless(struct timeval*, struct timeval*);
void          tvadd(struct timeval*, struct timeval*, struct timeval*);
void          tvsub(struct timeval*, struct timeval*, struct timeval*);
unsigned int  pop_count(unsigned int);
int           FD_POP(int, fd_set*);
int           FD_FFS(int, int, fd_set*);
int           FD_FFSandC(int, int, fd_set*);
int           create_tx_tcp(state_t *state,unsigned long, int);
int           send_data(state_t *state,fd_set*, fd_set*);


#endif /* _trident_util_h_ */

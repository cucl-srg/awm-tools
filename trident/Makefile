#******************************************************************************
#                                                                             *
#   Copyright 2006 Queen Mary University of London                            *
#   Copyright 2005 University of Cambridge Computer Laboratory.               *
#                                                                             *
#                                                                             *
#   This file is part of Trident.                                             *
#                                                                             *
#   Trident is free software; you can redistribute it and/or modify           *
#   it under the terms of the GNU General Public License as published by      *
#   the Free Software Foundation; either version 2 of the License, or         *
#   (at your option) any later version.                                       *
#                                                                             *
#   Trident is distributed in the hope that it will be useful,                 *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
#   GNU General Public License for more details.                              *
#                                                                             *
#   You should have received a copy of the GNU General Public License         *
#   along with Trident; if not, write to the Free Software                    *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
#                                                                             *
#*****************************************************************************/

CC      = gcc
CFLAGS  = -Wall -ggdb -DDEBUG_DISTN
LDFLAGS	= -lm -ggdb
ARFLAGS = -ruv
AR = ar

SRCS    = src/trident.c src/trident_util.c      	\
	src/simple_tx.c src/simple_rx.c			\
	src/simple_svr.c src/simple_clt.c		\
	src/cts_clt.c src/surge_clt.c 			\
	src/fileParser.c
STANDALONE = src/mainline.c

THREADTEST = src/threadline.c

OBJS = $(SRCS:.c=.o)
STANDALONE_OBJS = $(STANDALONE:.c=.o)
THREADTEST_OBJS = $(THREADTEST:.c=.o)

all: libtrident.a trident THtrident 

clean: 
	$(RM) core src/*.o *.txt libtrident.a bin/trident bin/THtrident \#*\# *~src/Dmakefile
	touch src/Dmakefile

libtrident.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

trident:libtrident.a $(STANDALONE_OBJS)
	$(CC) $(LDFLAGS) -o bin/$@ $(STANDALONE_OBJS) -L. -ltrident 

THtrident:libtrident.a $(THREADTEST_OBJS)
	$(CC) $(LDFLAGS) -o bin/$@ $(THREADTEST_OBJS) -L. -ltrident -lpthread

depend::
	@[ -f Dmakefile ] || touch Dmakefile
	makedepend -f Dmakefile -- $(CFLAGS) -- $(SRCS)  $(THREADTEST)\
	$(STANDALONE) 

include src/Dmakefile

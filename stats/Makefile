#*****************************************************************************
#                                                                            *
#  Copyright 2006 Queen Mary University of London                            *
#  Copyright 1995 University of Cambridge                                    *
#                                                                            *
#  This file is part of stats.                                                *
#                                                                            *
#  stats is free software; you can redistribute it and/or modify              *
#  it under the terms of the GNU General Public License as published by      *
#  the Free Software Foundation; either version 2 of the License, or         *
#  (at your option) any later version.                                       *
#                                                                            *
#  stats is distributed in the hope that it will be useful,                   *
#  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
#  GNU General Public License for more details.                              *
#                                                                            *
#  You should have received a copy of the GNU General Public License         *
#  along with Trident; if not, write to the Free Software                    *
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
#                                                                            *
#****************************************************************************/
#

C1FILES = check_stats.c  sampleHistogram.c  sampleStatistic.c
C2FILES = curve.c

CC = gcc

CFLAGS =  -ggdb -Wall -pedantic

O1FILES = $(C1FILES:.c=.o)
O2FILES = $(C2FILES:.c=.o)



all: check_stats curve

check_stats: ${O1FILES}
	${CC} ${CFLAGS} -o $@ ${O1FILES} -lm

curve: ${O2FILES}
	${CC} ${CFLAGS} -o $@ ${O2FILES} -lm

clean:
	rm -f ${O1FILES} ${O2FILES} check_stats curve core *~

depend: Dmakefile
	makedepend -f Dmakefile -- $(CFLAGS) --	$(C1FILES)

Dmakefile:
	@[ -f Dmakefile ] || touch Dmakefile

include Dmakefile

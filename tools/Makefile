## -*- mode: makefile -*-

##
## This file is part of the Score-P software (http://www.score-p.org)
##
## Copyright (c) 2014,
## Technische Universitaet Dresden, Germany
##
## This software may be modified and distributed under the terms of
## a BSD-style license.  See the COPYING file in the package base
## directory for details.
##

OTF2_CONFIG = /p/lscratchh/wang116/packages/otf2-2.1.1/build/bin/otf2-config
OTF2_LDFLAGS = `$(OTF2_CONFIG) --ldflags`
OTF2_CFLAGS = `$(OTF2_CONFIG) --cflags`
OTF2_LIBS = `$(OTF2_CONFIG) --libs`

RECORDER_CFLAGS = -I/g/g90/wang116/sources/Recorder/include

ALL = otf2-converter

all: $(ALL)

otf2-converter.o: otf2-converter.c
	mpicc -std=c99 $(OTF2_CFLAGS) $(RECORDER_CFLAGS) \
            -c otf2-converter.c \
            -o otf2-converter.o

otf2-converter: otf2-converter.o
	mpicc otf2-converter.o \
            $(OTF2_LDFLAGS) $(OTF2_LIBS) \
            -o otf2-converter

clean:
	rm $(ALL) *.o

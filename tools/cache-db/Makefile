# Copyright (C) 2018, 2019  Stefan Vargyas
# 
# This file is part of Cache-DB.
# 
# Cache-DB is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# Cache-DB is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Cache-DB.  If not, see <http://www.gnu.org/licenses/>.

.PHONY:: default clean allclean all depend

default: all

PROGRAM := cache
LIBRARY := libgdbm.so

# verify external parameters and
# update dependent variables

ifndef HASH
HASH_BITS := 32
else
ifneq ($(words ${HASH}),1)
$(error invalid HASH='${HASH}')
endif
ifneq ($(filter-out 32 64,${HASH}),)
$(error invalid HASH='${HASH}')
endif
HASH_BITS := ${HASH}
endif

# default GDBM paths

GDBM_HOME :=
GDBM_INCLUDE := 
GDBM_LIB := 

ifneq (${GDBM_HOME},)
GDBM_INCLUDE := ${GDBM_HOME}/include
GDBM_LIB := ${GDBM_HOME}/lib
endif

ifneq (${GDBM_INCLUDE},)
ifneq ($(shell bash -c 'test -d "${GDBM_INCLUDE}" && echo OK'),OK)
$(error GDBM include directory '${GDBM_INCLUDE}' not found)
endif
endif

ifneq (${GDBM_LIB},)
ifneq ($(shell bash -c 'test -d "${GDBM_LIB}" && echo OK'),OK)
$(error GDBM library directory '${GDBM_LIB}' not found)
endif
endif

# default GCC parameters

GCC := gcc
GCC_STD := gnu99

# obtain the actual version numbers of installed or custom GDBM

GDBM_VERSION = $(shell \
	bash -c '. libgdbm-version.sh && libgdbm-version ${GDBM_LIB} ${GCC}')

ifeq (${GDBM_VERSION},)
$(error cannot determine libgdbm's version)
endif

CFLAGS := -Wall -Wextra \
          -std=${GCC_STD} -Ilib \
          -DPROGRAM=${PROGRAM} \
          -DLIBRARY=${LIBRARY} \
          -DGDBM_VERSION=${GDBM_VERSION} \
          -DHASH_BITS=${HASH_BITS} \
          -DDEBUG -g \
          -lgdbm

ifneq (${GDBM_INCLUDE},)
CFLAGS += -I${GDBM_INCLUDE}
endif
ifneq (${GDBM_LIB},)
CFLAGS += -L${GDBM_LIB}
endif

SRCS := cache.c
BIN  := cache

# dependency rules

ifeq (.depend, $(wildcard .depend))
include .depend
endif

# building rules

${BIN}:
	${GCC} ${CFLAGS} ${SRCS} -o $@

# main targets

all: ${BIN}

clean:

allclean: clean
	rm -f ${BIN}

depend:
	${GCC} ${CFLAGS} -c ${SRCS} -MM| \
    sed -r 's/^[^ \t]+\.o:/${BIN}:/' > .depend



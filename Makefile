INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
//CFLAGS= -O3 -msse3 -malign-double -g $(INCLUDE_DIRS) $(CDEFS)
LIBS=

DRIVER=raidtest

HFILES= raidlib.h
CFILES= raidtest.c raidlib.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${DRIVER}

clean:
	-rm -f *.o *.NEW *~
	-rm -f ${DRIVER} ${DERIVED} ${GARBAGE}

${DRIVER}:	${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<

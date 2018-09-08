CC = gcc
CFLAGS =  -g
IFLAGS = -I.
SRCS = server.c 
OBJS = server.o 

all: server

server: server.o
	gcc -o server server.o
	
clean:
	rm server $(OBJS)
 
depend:
	makedepend -- ${IFLAGS} -- ${LDFLAGS} -- $(SRCS)
# DO NOT DELETE THIS LINE - maketd DEPENDS ON IT
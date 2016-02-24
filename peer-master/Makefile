CC = gcc
CFLAGS = -w 
PROG = peer

SRCS = peer.c md5.c

all: $(PROG)

$(PROG):	$(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS)

clean:
	rm -f $(PROG)

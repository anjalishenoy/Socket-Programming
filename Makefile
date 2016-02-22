CC = gcc
CFLAGS = -w 
PROG = peer

SRCS = main.c md5.c

all: $(PROG)

$(PROG):	$(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS)

clean:
	rm -f $(PROG)

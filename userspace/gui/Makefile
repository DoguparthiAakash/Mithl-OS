CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -static

all: compositor

compositor: compositor.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o compositor compositor.c

clean:
	rm -f compositor

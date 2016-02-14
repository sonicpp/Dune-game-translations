CC=gcc
CFLAGS=-std=c99 -Wall
EXEC=tu

all: $(EXEC)

$(EXEC): tu.o
	$(CC) tu.o -o $@
tu.o: tu.c
	$(CC) $(CFLAGS) tu.c -c

clean:
	rm -f *.o $(EXEC)

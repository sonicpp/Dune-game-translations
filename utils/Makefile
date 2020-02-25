CC=gcc
CFLAGS=-std=c99 -Wall
BIN_TU=tu
BIN_HSQ=hsq
BIN_SPRITE=sprite

all: $(BIN_TU) $(BIN_HSQ) $(BIN_SPRITE)

$(BIN_TU): tu.o
	$(CC) tu.o -o $@
tu.o: tu.c
	$(CC) $(CFLAGS) tu.c -c
$(BIN_HSQ): hsq.o
	$(CC) hsq.o -o $@
hsq.o: hsq.c
	$(CC) $(CFLAGS) hsq.c -c
$(BIN_SPRITE): sprite.o
	$(CC) sprite.o -o $@
sprite.o: sprite.c
	$(CC) $(CFLAGS) sprite.c -c

clean:
	rm -f *.o $(BIN_TU) $(BIN_HSQ) $(BIN_SPRITE)

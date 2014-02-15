INC=-I./include
CFLAGS=-Wall -std=c99 $(INC)
VPATH=src:obj

build: quicklz.o

%.o: %.c
	$(CC) -c -o obj/$@ $< $(CFLAGS)
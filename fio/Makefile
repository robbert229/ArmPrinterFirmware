AR=ar
CC=gcc
CROSS=arm-none-linux-gnueabi-
INC=-I./include -I../ -I../quicklz
CFLAGS=-Wall -std=c99 $(INC)
VPATH=

build: libfioarm.a libfiowin.a

clean: 
	-rm -f fioarm.o fiowin.o libfioarm.a libfiowin.a

libfioarm.a: fioarm.o
	$(CROSS)$(AR) -cvq libfioarm.a fioarm.o

libfiowin.a: fiowin.o
	$(AR) -cvq libfiowin.a fiowin.o

fiowin.o: fio.c
	$(CC) -c -o fiowin.o fio.c $(CFLAGS)

fioarm.o: fio.c
	$(CROSS)$(CC) -c -o fioarm.o fio.c $(CFLAGS)
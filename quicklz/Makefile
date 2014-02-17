AR=ar
CC=gcc
CROSS=arm-none-linux-gnueabi-
INC=-I./include
CFLAGS=-Wall -std=c99 $(INC)
VPATH=

build: libquicklzarm.a libquicklzwin.a

clean: 
	-rm -f quicklzarm.o quicklzwin.o libquicklzarm.a libquicklzwin.a

libquicklzarm.a: quicklzarm.o
	$(CROSS)$(AR) -cvq libquicklzarm.a quicklzarm.o

libquicklzwin.a: quicklzwin.o
	$(AR) -cvq libquicklzwin.a quicklzwin.o

quicklzwin.o: quicklz.c
	$(CC) -c -o quicklzwin.o quicklz.c

quicklzarm.o: quicklz.c
	$(CROSS)$(CC) -c -o quicklzarm.o quicklz.c
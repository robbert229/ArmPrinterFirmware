CROSS=arm-none-linux-gnueabi-
CC=gcc

INC=-I./include -I../common/include
CFLAGS=-Wall -std=c99 $(INC)
LDFLAGS=-lm
VPATH = src:obj:../common/obj:bin

build: ARMStrongPrinting.exe

ARMStrongPrinting.exe: ARMStrongPrinting.o quicklz.o breshenham.o io.o
	$(CC) $^ -o bin/ARMStrongPrinting.exe $(LDFLAGS)

%.o: %.c
	$(CC) -c -o obj/$@ $< $(CFLAGS)
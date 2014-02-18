TOPDIR  := ./
OBJDIR  := $(TOPDIR)obj/
BINDIR  := $(TOPDIR)bin/
NAME	:= ARMStrongHost
EXE     := $(BINDIR)$(NAME)

SFILES  := c
OFILES  := o
CC      := gcc
INC		:= -I. -I../quicklz -I../fio -I../
CFLAGS  := -c -Wall -std=c99 $(INC)
LDFLAGS := -L../quicklz -lquicklzwin -L../fio -lfiowin
SOURCES := $(wildcard *.$(SFILES))
OBJECTS := $(patsubst %$(SFILES), $(OBJDIR)%$(OFILES), $(SOURCES))

ALLFILES := $(SOURCES)

build:    $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJDIR)%$(OFILES):    %$(SFILES)
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm -f $(OBJECTS) $(EXE)
	
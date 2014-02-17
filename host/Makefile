TOPDIR  := ./
OBJDIR  := $(TOPDIR)obj/
BINDIR  := $(TOPDIR)bin/
NAME	:= ARMStrongHost
EXE     := $(BINDIR)$(NAME)

SFILES  := c
OFILES  := o
CC      := gcc
INC		:= -I. -I../quicklz
CFLAGS  := -c -Wall -std=c99 $(INC)
LDFLAGS := -L../quicklz -lquicklzwin
SOURCES := $(wildcard *.$(SFILES))
OBJECTS := $(patsubst %$(SFILES), $(OBJDIR)%$(OFILES), $(SOURCES))

ALLFILES := $(SOURCES)

build:    $(EXE)

$(EXE): $(OBJECTS)
	@$(CC) $^ -o $@ $(LDFLAGS)
	@echo "CC:$@"

$(OBJDIR)%$(OFILES):    %$(SFILES)
	@$(CC) $(CFLAGS) $< -o $@
	@echo "CC:$@"

clean:clean_obj build
clean_obj:
	@-rm -f $(OBJECTS) $(EXE)
	@echo "Cleaned"
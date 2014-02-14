CROSS=arm-none-linux-gnueabi-
CC=gcc

SRC=./src/
BIN=./bin/

FLAGS=-Wall -std=c99 -lm

build: $(BIN)ARMStrongPrinting.exe $(BIN)ARMStrongFiq $(BIN)ARMStrongSplitter.exe $(BIN)ARMStrongSplitter $(BIN)ARMStrongPrinting

clean:
	-del $(BIN)ARMStrongPrinting.exe
	-del $(BIN)ARMStrongFiq.exe
	-del $(BIN)ARMStrongSplitter.exe
	-del $(BIN)ARMStrongSplitter
	-del $(BIN)ARMStrongPrinting
	-del $(BIN)ARMStrongFiq

#desktop stuff
$(BIN)ARMStrongPrinting.exe:$(SRC)ARMStrongPrinting.c $(SRC)quicklz.c
	$(CC) $(SRC)ARMStrongPrinting.c $(SRC)quicklz.c -o $(BIN)ARMStrongPrinting.exe $(FLAGS)

$(BIN)ARMStrongSplitter.exe:$(SRC)ARMStrongSplitter.c
	$(CC) $(SRC)ARMStrongSplitter.c -o $(BIN)ARMStrongSplitter.exe $(FLAGS)


#embedded stuff
$(BIN)ARMStrongPrinting:$(SRC)ARMStrongPrinting.c $(SRC)quicklz.c
	$(CROSS)$(CC) $(SRC)ARMStrongPrinting.c $(SRC)quicklz.c -o $(BIN)ARMStrongPrinting $(FLAGS)

$(BIN)ARMStrongFiq:$(SRC)ARMStrongFiq.c $(SRC)quicklz.c
	$(CROSS)$(CC) $(SRC)ARMStrongFiq.c $(SRC)quicklz.c -o $(BIN)ARMStrongFiq $(FLAGS)

$(BIN)ARMStrongSplitter:$(SRC)ARMStrongSplitter.c
	$(CROSS)$(CC) $(SRC)ARMStrongSplitter.c -o $(BIN)ARMStrongSplitter


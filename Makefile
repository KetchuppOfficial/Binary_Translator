CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wshadow -Wfloat-equal -Wswitch-default

DEBUG = -g

SRC = src/
BIN = bin/

OBJECTS = main.o Binary_Translator.o
MY_LIB  = ../../My_Lib/My_Lib.a

all: Binary_Translator

Binary_Translator: $(OBJECTS)
	$(CC) $(BIN)main.o $(BIN)Binary_Translator.o $(MY_LIB) -o $(BIN)Binary_Translator.out
	rm $(BIN)*.o

main.o:
	$(CC) $(DEBUG) $(CFLAGS) -c $(SRC)main.c -o $(BIN)main.o

Binary_Translator.o:
	$(CC) $(DEBUG) $(CFLAGS) -c $(SRC)Binary_Translator.c -o $(BIN)Binary_Translator.o

run:
	$(BIN)Binary_Translator.out $(IN) $(OUT)

clean:
	rm -rf *.o
	rm -rf *.out
	rm -rf *.log

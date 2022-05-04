CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wshadow -Wfloat-equal -Wswitch-default

DEBUG = -g

SRC = Source/
OBJ = Objects/

OBJECTS = main.o Binary_Translator.o
MY_LIB  = ../../My_Lib/My_Lib.a

all: Binary_Translator

Binary_Translator: $(OBJECTS)
	$(CC) $(OBJECTS) $(MY_LIB) -o Binary_Translator.out
	rm $(OBJECTS)

main.o:
	$(CC) $(DEBUG) $(CFLAGS) -c $(SRC)main.c              -o main.o

Binary_Translator.o:
	$(CC) $(DEBUG) $(CFLAGS) -c $(SRC)Binary_Translator.c -o Binary_Translator.o

run:
	./Binary_Translator.out $(IN) $(OUT)

clean:
	rm -rf *.o
	rm -rf *.out
	rm -rf *.log

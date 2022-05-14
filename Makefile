CC     = gcc
CFLAGS = -Wall -Werror -Wshadow -Wfloat-equal -Wswitch-default

DEBUG = -g

MY_LIB  = ~/Programming/My_Lib/My_Lib.a

BIN      = ./bin/
SRCDIR   = ./src/
BUILDDIR = ./build/

SRC_LIST = main.c Binary_Translator.c #ELF_Generator.c 
SRC = $(addprefix $(SRCDIR),$(SRC_LIST))

SUBS := $(SRC)
SUBS := $(subst $(SRCDIR), $(BUILDDIR), $(SUBS))

OBJ  = $(SUBS:.c=.o)
DEPS = $(SUBS:.c=.d)

.PHONY: all
all: $(DEPS) $(OBJ)
	@mkdir -p $(BIN)
	@echo "Linking project..."
	@$(CC) $(OBJ) $(SHA_LIB) $(MY_LIB) -o $(BIN)Binary_Translator.out

$(BUILDDIR)%.o: $(SRCDIR)%.c
	@mkdir -p $(dir $@)
	@echo "Compiling \"$<\"..."
	@$(CC) $(CFLAGS) $(DBG) $(OPT) -c $< -o $@

include $(DEPS)

$(BUILDDIR)%.d: $(SRCDIR)%.c
	@echo "Collecting dependencies for \"$<\"..."
	@mkdir -p $(dir $@)
	@$(CC) -E $(CFLAGS) $< -MM -MT $(@:.d=.o) > $@

.PHONY: run clean

clean:
	rm -rf $(OBJ) $(DEPS)

run: $(BIN)Binary_Translator.out
	@echo "Running \"$<\"..."
	@$(BIN)Binary_Translator.out $(IN) $(OUT)
	@echo "Completed"

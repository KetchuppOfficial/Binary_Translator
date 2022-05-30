CC     = gcc
CFLAGS = -Wall -Werror -Wshadow -Wfloat-equal -Wswitch-default

BIN      = ./bin/
SRCDIR   = ./src/
BUILDDIR = ./build/

SRC_LIST = main.c Binary_Translator.c
SRC = $(addprefix $(SRCDIR), $(SRC_LIST))

SUBS := $(SRC)
SUBS := $(subst $(SRCDIR), $(BUILDDIR), $(SUBS))

OBJ  = $(SUBS:.c=.o)
DEPS = $(SUBS:.c=.d)

LIBS_LIST = My_Lib
LIBSDIR = $(addprefix ./lib/, $(LIBS_LIST))
LIBS = $(addsuffix /*.a, $(LIBSDIR))

.PHONY: all $(LIBSDIR)

all: $(DEPS) $(OBJ) $(LIBSDIR)
	@mkdir -p $(BIN)
	@echo "Linking project..."
	@$(CC) $(OBJ) $(LIBS) -o $(BIN)Binary_Translator.out

$(LIBSDIR):
	@$(MAKE) -C $@ --no-print-directory -f Makefile.mak

$(BUILDDIR)%.o: $(SRCDIR)%.c
	@mkdir -p $(dir $@)
	@echo "Compiling \"$<\"..."
	@$(CC) $(CFLAGS) -g $(OPT) -c -I$(LIBSDIR) $< -o $@

include $(DEPS)

$(BUILDDIR)%.d: $(SRCDIR)%.c
	@echo "Collecting dependencies for \"$<\"..."
	@mkdir -p $(dir $@)
	@$(CC) -E $(CFLAGS) -I$(LIBSDIR) $< -MM -MT $(@:.d=.o) > $@

.PHONY: run clean

clean:
	rm -rf $(OBJ) $(DEPS)

run: $(BIN)Binary_Translator.out
	@echo "Running \"$<\"..."
	@$(BIN)Binary_Translator.out $(IN)

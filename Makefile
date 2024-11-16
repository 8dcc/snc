
CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Wpedantic
LDLIBS=

SRC=main.c
OBJ=$(addprefix obj/, $(addsuffix .o, $(SRC)))

BIN=snc

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

#-------------------------------------------------------------------------------

.PHONY: all clean install

all: $(BIN)

clean:
	rm -f $(OBJ)
	rm -f $(BIN)

install: $(BIN)
	mkdir -p $(BINDIR)
	install -m 755 $^ $(BINDIR)

#-------------------------------------------------------------------------------

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

obj/%.c.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

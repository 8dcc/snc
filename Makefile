
CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -Wpedantic -Wshadow
LDLIBS=

SRC=main.c util.c args.c receive.c transmit.c
OBJ=$(addprefix obj/, $(addsuffix .o, $(SRC)))

BIN=snc
COMPLETION=snc-completion.bash

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
COMPLETIONDIR=$(PREFIX)/share/bash-completion/completions

#-------------------------------------------------------------------------------

.PHONY: all clean install install-bin install-completion

all: $(BIN)

clean:
	rm -f $(OBJ)
	rm -f $(BIN)

install: install-bin install-completion

install-bin: $(BIN)
	install -D -m 755 $^ -t $(DESTDIR)$(BINDIR)

install-completion: $(COMPLETION)
	install -D -m 644 $^ $(DESTDIR)$(COMPLETIONDIR)/$(BIN)

#-------------------------------------------------------------------------------

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

obj/%.c.o : src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

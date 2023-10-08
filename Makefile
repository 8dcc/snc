
CC=gcc
CFLAGS=-Wall -Wextra
LDFLAGS=

OBJ_FILES=main.c.o
OBJS=$(addprefix obj/, $(OBJ_FILES))

INSTALL_DIR=/usr/local/bin
BIN=snc

.PHONY: clean all install

# -------------------------------------------

all: $(BIN)

clean:
	rm -f $(OBJS)
	rm -f $(BIN)

install: $(BIN)
	mkdir -p $(INSTALL_DIR)
	install -m 755 ./$(BIN) $(INSTALL_DIR)/$(BIN)

# -------------------------------------------

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): obj/%.c.o : src/%.c
	@mkdir -p obj/
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)


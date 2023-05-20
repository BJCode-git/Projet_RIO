LIBS= -pthread #-lSDL2
CC=gcc
LD=gcc
SRCS=$(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=object/%.o)
CFLAGS = -W -Wall -Wextra -Wno-implicit-fallthrough -Wunused-result #-Werror
PROG = client proxy server
BIN = $(PROG)

.PHONY: all create_bin_dir docs clean_docs clean $(PROG)

create_bin_dir:
	mkdir -p object
	mkdir -p bin

all: create_bin_dir server client proxy correcteur docs

server: include/base.h src/correcteur.c src/server.c
	$(CC) -o bin/$@ $^ -g $(LIBS)

client: include/base.h src/correcteur.c src/client.c
	$(CC) -o bin/$@ $^ -g $(LIBS)

proxy: include/base.h src/proxy.c
	$(CC) -o bin/$@ $^ -g $(LIBS)

correcteur: src/correcteur.c
	$(CC) -o bin/$@ $^ -D TEST_CRC -g 


docs:
	doxygen Doxyfile

clean_docs:
	rm -r -f doc/*

clean:
	rm -f $(BIN) $(OBJS)
	rm -f bin/$(PROG)


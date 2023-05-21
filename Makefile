LIBS= -pthread #-lSDL2
CC=gcc
LD=gcc
SRCS=$(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=object/%.o)
CFLAGS = -W -Wall -Wextra -Wno-unused-parameter -Werror -pedantic -fsanitize=thread -Wno-implicit-fallthrough -g
PROG = client proxy server
BIN = $(PROG)

.PHONY: all create_bin_dir docs clean_docs clean $(PROG)


all: create_bin_dir server client proxy correcteur

create_bin_dir:
	mkdir -p bin
	mkdir -p docs

server: include/base.h src/correcteur.c src/server.c
	$(CC) -o bin/$@ $^ $(CFLAGS) $(LIBS)

client: include/base.h src/correcteur.c src/client.c
	$(CC) -o bin/$@ $^ $(CFLAGS) $(LIBS)

proxy: include/base.h src/correcteur.c src/proxy.c
	$(CC) -o bin/$@ $^ $(CFLAGS) $(LIBS)

correcteur: src/correcteur.c
	$(CC) -o bin/$@ $^ -D TEST_CRC $(CFLAGS)

create_sh:
	mkdir -p bin
	echo "#!/bin/bash\n./bin/server\nsleep 30" > bin/server.sh
	chmod +x bin/server.sh
	echo "#!/bin/bash\n./bin/client 127.0.0.1\nsleep 30" > bin/client.sh
	chmod +x bin/client.sh
	echo "#!/bin/bash\n./bin/proxy bin/server.txt\nsleep 30" > bin/proxy.sh
	chmod +x bin/proxy.sh

create_config_example:
	mkdir -p bin
	echo -n "127.0.0.1" > bin/server.txt
	echo -n "127.0.0.1:54321\n127.0.0.1:54123" > bin/servers.txt


test: all create_sh create_config_example 
	gnome-terminal -- ./bin/server.sh
	gnome-terminal -- ./bin/proxy.sh
	gnome-terminal -- ./bin/client.sh

docs:
	doxygen Doxyfile

clean_docs:
	rm -r -f docs/*

clean:
	rm -f $(BIN) $(OBJS)
	rm -f bin/*
	rmdir bin


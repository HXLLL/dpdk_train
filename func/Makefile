LIB_FILE := ../utils/*.c

PKGCONF ?= pkg-config

CC := gcc
CFLAGS += -g3 $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS = $(shell $(PKGCONF) --libs libdpdk)

.PHONY: all
all: build/main

build/main: client.c server.c main.c utils.c $(LIB_FILE)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f build/*

$(shell mkdir -p build)
LIB_FILE := ../utils/*.c

PKGCONF ?= pkg-config

CFLAGS += -g3 $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS = $(shell $(PKGCONF) --libs libdpdk)

.PHONY: all
all: build/func_client build/func_server

build/func_client: func_client.c $(LIB_FILE) Makefile
	$(CC) $(CFLAGS) $(LIB_FILE) func_client.c -o $@ $(LDFLAGS)

build/func_server: func_server.c $(LIB_FILE) Makefile
	$(CC) $(CFLAGS) $(LIB_FILE) func_server.c -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f build/*

$(shell mkdir -p build)
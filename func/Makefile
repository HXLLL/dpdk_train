LIB_FILE := ../utils/*.c

PKGCONF ?= pkg-config

CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS = $(shell $(PKGCONF) --libs libdpdk)

.PHONY: all
all: func_client func_server

func_client: func_client.c $(LIB_FILE) Makefile
	$(CC) $(CFLAGS) $(LIB_FILE) func_client.c -o $@ $(LDFLAGS)

func_server: func_server.c $(LIB_FILE) Makefile
	$(CC) $(CFLAGS) $(LIB_FILE) func_server.c -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f build/$(APP)
	test -d build && rmdir -p build || true

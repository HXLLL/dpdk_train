LIB_FILE := ../utils/*.c

PKGCONF ?= pkg-config

CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS = $(shell $(PKGCONF) --libs libdpdk)

.PHONY: all
all: fib_client fib_server

fib_client: fib_client.c $(LIB_FILE) Makefile
	$(CC) $(CFLAGS) $(LIB_FILE) fib_client.c -o $@ $(LDFLAGS)

fib_server: fib_server.c $(LIB_FILE) Makefile
	$(CC) $(CFLAGS) $(LIB_FILE) fib_server.c -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f build/$(APP)
	test -d build && rmdir -p build || true

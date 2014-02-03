CPPFLAGS ?=
CFLAGS   ?= -Os -Wall -pedantic -std=gnu11
LDFLAGS  ?= -s

init: init.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm init

.PHONY: clean

CPPFLAGS ?=
CFLAGS   ?= -Os -Wall -pedantic -std=gnu11
LDFLAGS  ?= -s

minit: minit.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm minit

.PHONY: clean

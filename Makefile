CPPFLAGS =
CFLAGS   = -Os -Wall -pedantic
LDFLAGS  = -s

DATE   := $(shell date +%Y%m%d)
VERSION = $(DATE)

BIN = minit
SRC = $(BIN).c

DIST_BASE = $(BIN)-$(VERSION)
DIST      = $(DIST_BASE).tar.gz

prefix      =
exec_prefix = $(prefix)
sbindir     = $(exec_prefix)/sbin


all: $(BIN)
$(BIN): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=gnu99 $^ $(LDFLAGS) -o $@

clean:
	rm -f $(BIN)

install: $(BIN)
	install -D $(BIN) $(DESTDIR)$(sbindir)/$(BIN)
uninstall:
	rm -f $(DESTDIR)$(sbindir)/$(BIN)

dist: $(DIST)
$(DIST): $(SRC) COPYING Makefile README.md example/Dockerfile example/startup
	mkdir $(DIST_BASE)
	cp -a --parents $^ $(DIST_BASE)
	tar czf $@ $(DIST_BASE)
	rm -rf $(DIST_BASE)

.PHONY: all clean install uninstall dist

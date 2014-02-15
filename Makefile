CPPFLAGS =
CFLAGS   = -Os -Wall -pedantic
LDFLAGS  = -s

DATE   := $(shell date +%Y%m%d)
VERSION = $(DATE)

all: minit
minit: minit.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=gnu11 $^ $(LDFLAGS) -o $@

clean:
	rm -f minit

dist: minit-$(VERSION).tar.gz
minit-$(VERSION).tar.gz: COPYING Makefile README.md minit.c example/Dockerfile example/startup
	set -e; mkdir ${@:%.tar.gz=%}; cp -a --parents $^ ${@:%.tar.gz=%}; \
		tar czf $@ ${@:%.tar.gz=%}; rm -rf ${@:%.tar.gz=%};

.PHONY: all clean dist

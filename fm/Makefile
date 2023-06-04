VERSION = 1.0
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/man

NOICELDLIBS = -lcurses
NOICEOBJ = noice.o
BIN = noice
MAN = noice.1

CFLAGS = -pedantic -Wall -Os -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L

all: $(BIN)

noice: $(NOICEOBJ)
	$(CC) $(CFLAGS) -o $@ $(NOICEOBJ) $(LDFLAGS) $(NOICELDLIBS)

noice.o: noiceconf.h

noiceconf.h:
	cp noiceconf.def.h $@

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(BIN) $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	cp -f $(MAN) $(DESTDIR)$(MANPREFIX)/man1

uninstall:
	cd $(DESTDIR)$(PREFIX)/bin && rm -f $(BIN)
	cd $(DESTDIR)$(MANPREFIX)/man1 && rm -f $(MAN)

dist: clean
	mkdir -p noice-$(VERSION)
	cp `find . -maxdepth 1 -type f` noice-$(VERSION)
	tar -c noice-$(VERSION) | gzip > noice-$(VERSION).tar.gz

clean:
	rm -f $(BIN) $(NOICEOBJ) noice-$(VERSION).tar.gz
	rm -rf noice-$(VERSION)

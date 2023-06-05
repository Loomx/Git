PKG = fm
SRC = fm.c
VERSION = 1.0
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/man

LDLIBS = -lcurses
BIN = fm
MAN = fm.1

CFLAGS = -Wall -pedantic -Os -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L

$(PKG):
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $(LDLIBS) $(SRC)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(BIN) $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	cp -f $(MAN) $(DESTDIR)$(MANPREFIX)/man1

uninstall:
	cd $(DESTDIR)$(PREFIX)/bin && rm -f $(BIN)
	cd $(DESTDIR)$(MANPREFIX)/man1 && rm -f $(MAN)

dist: clean
	mkdir -p fm-$(VERSION)
	cp `find . -maxdepth 1 -type f` fm-$(VERSION)
	tar -c fm-$(VERSION) | gzip > fm-$(VERSION).tar.gz

clean:
	rm -f $(BIN) fm-$(VERSION).tar.gz
	rm -rf fm-$(VERSION)

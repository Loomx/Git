INSTALL=install
PREFIX=/usr/local
MANDIR?=/man/man1

TARGET := xcape

CFLAGS += -Wall
CFLAGS += `pkg-config --cflags xtst x11`
LDFLAGS += `pkg-config --libs xtst x11`
LDFLAGS += -pthread

$(TARGET): xcape.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install: $(TARGET)
	$(INSTALL) -d -m 0755 $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -d -m 0755 $(DESTDIR)$(PREFIX)$(MANDIR)
	$(INSTALL) -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	$(INSTALL) -m 0644 xcape.1 $(DESTDIR)$(PREFIX)$(MANDIR)/xcape.1

clean:
	rm $(TARGET)

uninstall:
	rm -f $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	rm -f $(DESTDIR)$(PREFIX)$(MANDIR)/xcape.1

.PHONY: install clean uninstall

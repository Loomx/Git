PKG = fm
SRC = fm.c
CC  = cc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -lncurses
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/man
MAN = fm.1


$(PKG): $(SRC)
	${CC} ${CFLAGS} ${SRC} -o $@

install: $(PKG)
	@echo installing executable to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f ${PKG} ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/${PKG}
	@mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	@cp -f $(MAN) $(DESTDIR)$(MANPREFIX)/man1

clean:
	@echo cleaning
	@rm -f ${PKG}

uninstall:
	@echo removing executable from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/${PKG}
	@rm -f $(DESTDIR)$(MANPREFIX)/man1/$(MAN) 

.PHONY: install clean uninstall

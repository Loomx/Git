PKG = player
SRC = player.c
CC  = cc
CFLAGS = -pedantic -Wall
PREFIX = /usr/local

${PKG}: ${SRC}
	${CC} ${CFLAGS} ${SRC} -o $@

install: ${PKG}
	@echo installing executable to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f ${PKG} ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/${PKG}

clean:
	@echo cleaning
	@rm -f ${PKG}

uninstall:
	@echo removing executable from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/${PKG}

.PHONY: install clean uninstall

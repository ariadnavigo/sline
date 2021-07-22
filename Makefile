# See LICENSE file for copyright and license details.

.POSIX:

include config.mk

SRC = history.c sline.c strlcpy.c
OBJ = ${SRC:%.c=%.o}
MAN = sline.3 sline_end.3 sline_errmsg.3 sline_setup.3

all: options libsline.a

options:
	@echo Build options:
	@echo "CPPFLAGS = ${CPPFLAGS}"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	@echo

history.o: strlcpy.h

sline.o: history.h sline.h strlcpy.h

strlcpy.o: strlcpy.h

${OBJ}: config.mk

libsline.a: ${OBJ}
	ar -rcs $@ ${OBJ}

clean:
	rm -f libsline.a ${OBJ} sline-${VERSION}.tar.gz

dist: clean
	mkdir -p sline-${VERSION}
	cp -R LICENSE Makefile README.md history.h sline.h strlcpy.h ${MAN} \
	   ${SRC} sline-${VERSION}
	tar -cf sline-${VERSION}.tar sline-${VERSION}
	gzip sline-${VERSION}.tar
	rm -rf sline-${VERSION}

install-man: ${MAN}
	mkdir -p ${DESTDIR}${MANPREFIX}/man3
	for manpage in ${MAN}; do \
		sed "s/VERSION/${VERSION}/g" $$manpage \
		    > ${DESTDIR}${MANPREFIX}/man3/$$manpage ; \
	done
	chmod 644 ${DESTDIR}${MANPREFIX}/man3/sline_*

install: all install-man
	mkdir -p ${DESTDIR}${PREFIX}/lib
	cp -f libsline.a ${DESTDIR}${PREFIX}/lib
	chmod 644 ${DESTDIR}${PREFIX}/lib/libsline.a
	mkdir -p ${DESTDIR}${PREFIX}/include
	cp -f sline.h ${DESTDIR}${PREFIX}/include
	chmod 644 ${DESTDIR}${PREFIX}/include/sline.h

uninstall:
	rm -f ${DESTDIR}${PREFIX}/lib/libsline.a \
	   ${DESTDIR}${PREFIX}/include/sline.h \
	   ${DESTDIR}${MANPREFIX}/man3/sline*

.PHONY: all options clean dist install install-man uninstall

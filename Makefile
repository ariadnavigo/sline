# See LICENSE file for copyright and license details.

.POSIX:

include config.mk

SRC = history.c sline.c strlcpy.c
OBJ = ${SRC:%.c=%.o}
MAN = man/sline.3 man/sline_end.3 man/sline_errmsg.3 man/sline_setup.3 \
      man/sline_set_prompt.3 man/sline_version.3

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

# sline_test: Test program which will always use the development version of 
# sline as compiled in the source directory, never the library installed on the 
# system. This program isn't installed by `make install'.
sline_test: options libsline.a sline.h sline_test.c
	${CC} ${CFLAGS} ${CPPFLAGS} -o $@ sline_test.c libsline.a

clean:
	rm -f libsline.a sline_test ${OBJ}

install-man:
	mkdir -p ${DESTDIR}${MANPREFIX}/man3
	for manpage in ${MAN}; do \
		sed "s/VERSION/${VERSION}/g" $$manpage \
		    > ${DESTDIR}${MANPREFIX}/man3/$${manpage#man/} ; \
	done
	chmod 644 ${DESTDIR}${MANPREFIX}/man3/sline*

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

.PHONY: all options clean install install-man uninstall

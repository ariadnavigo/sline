# sline version
MAJOR = 1
VERSION = ${MAJOR}.0.1

LIBFULLNAME = libsline.so.${VERSION}
LIBSONAME = libsline.so.${MAJOR}

# Customize below to your needs

# Paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/man

# Flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_POSIX_C_SOURCE=200809L
#CFLAGS = -g -std=c99 -Wpedantic -Wall -Wextra
CFLAGS = -std=c99 -Wpedantic -Wall -Wextra
LDFLAGS = -shared -Wl,-soname,${LIBSONAME}

# Compiler and linker
CC = cc


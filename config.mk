# sline version
VERSION = 0.5.0

# Customize below to your needs

# Paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/man

# Flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_POSIX_C_SOURCE=200809L
#CFLAGS = -g -std=c99 -Wpedantic -Wall -Wextra
CFLAGS = -std=c99 -Wpedantic -Wall -Wextra
LDFLAGS =

# Compiler and linker
CC = cc


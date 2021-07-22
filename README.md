# sline - Simple line-editing and command history library

sline is a simple library that easily allows programs to as for user input 
with support for line-editing and command history. It implements a simple
VT100 compatible driver that makes it totally independent of third-party 
libraries.

sline was originally part of [scalc](https://sr.ht/~arivigo/scalc).

**This software is still in a very early stage of development. Expect many
changes in the near future!**

## Build

sline requires:

1. A POSIX-like system
2. A C99 compiler

Build by using:

```
$ make
```

Customize the build process by changing ``config.mk`` to suit your needs.

### No dynamic library version support

sline only provides a static library. It's out of the goals of this project to
implement a dynamic library version of it. Static libraries allow easier
integration into testing and are easier to deploy in general.

## Install

You may install sline by running the following command as root:

```
# make install
```

This will install the binary under ``$PREFIX/bin``, as defined by your
environment, or ``/usr/local/bin`` by default. The Makefile supports the
``$DESTDIR`` variable as well.

## API manuals

sline's API is very simple. You may refer to the sline(3) manual page for an
overview on how to use its subroutines. 

## Contributing

All contributions are welcome! If you wish to send in patches, ideas, or report
a bug, you may do so by sending an email to the
[sline-devel](https://lists.sr.ht/~arivigo/sline-devel) mailing list.

If interested in getting some news from the project, you may also want to
subscribe to the low-volume
[sline-announce](https://lists.sr.ht/~arivigo/sline-announce) mailing list!

## License

sline is published under an MIT/X11/Expat-type License. See ``LICENSE`` file
for copyright and license details.

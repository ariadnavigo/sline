# sline - Simple line-editing and command history library

sline is a simple library that easily allows programs to ask for user input 
with support for line-editing and command history. It implements a simple
VT100 compatible driver that makes it totally independent of third-party 
libraries. sline also supports UTF-8 in user input.

sline was originally part of [scalc](https://sr.ht/~arivigo/scalc).

## Build

sline requires:

1. A POSIX-like system
2. A C99 compiler

Build by using:

```
$ make
```

Customize the build process by changing ``config.mk`` to suit your needs.

## Install

You may install sline by running the following command as root:

```
# make install
```

This will install both a static and a dynamic library under ``$PREFIX/lib``, as 
defined by your environment, or ``/usr/local/lib`` by default. Header files 
will be installed under ``${PREFIX}/include``, or ``/usr/local/include`` by 
default. The Makefile supports the ``$DESTDIR`` variable as well.

Depending on your platform you might need to recreate the linker cache to make
the dynamic library available to your system. Usually, this is performed via
``ldconfig(8)``, but refer to your system's documentation.

sline is fully implemented in one single module with one single associated
header, namely ``sline.c`` and ``sline.h``, respectively. This also makes the
library easily used as-is by including it as a module into your project. If
you want to follow this route, make sure your project has access to a
compatible implementation of ``strlcpy()`` or make use of the one included in 
sline itself.

## API manuals

sline's API is very simple. You may refer to the sline(3) manual page for an
overview on how to use its main subroutine. Other subroutines are listed under 
the _See also_ section on that manual page.

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

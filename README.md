# sline - Simple line-editing and user input history library

sline is a simple library that easily allows programs to ask for user input 
with support for line-editing and user input history. It implements a simple
VT100 compatible driver that makes it totally independent of third-party 
libraries. sline also supports UTF-8 in user input.

sline was originally part of [scalc](https://github.com/ariadnavigo/scalc).

## Basic usage

A basic use example for sline is provided as ``sline_test.c`` in this 
repository, but in a nutshell, after setting up the terminal with 
``sline_setup()``, ``sline()`` will read a line from standard input while 
providing the user with line-editing features. ``sline_end()`` restores the
terminal back to normal and frees all memory used by sline.

The prototypes for the three subroutines mentioned above are:

```
/* 
 * sline_setup(): Sets the terminal up. The hist parameter sets the size of 
 * user input history buffers; a value of zero (0) will disable the history
 * feature altogether.
 */
int sline_setup(int hist);

/*
 * sline(): Open an sline prompt. User input is stored in buf, reading at most
 * as many characters as represented by size. init stores a default value that
 * may be shown at the prompt; NULL means no default value is shown (i.e. a
 * blank prompt).
 */
int sline(char *buf, int size, const char *init);

/*
 * sline_end(): Restore the terminal to its initial configuration and frees all
 * memory used by sline.
 */
void sline_end(void);
```

This little snippet illustrates the basic usage of sline:

```
	/* Set up the terminal without history support */
	if (sline_setup(0) < 0) {
		/* Setup failed, take appropriate actions */
	}

	if ((sline_stat = sline(buf, BUF_SIZE, INIT_STR)) < 0) {
		/* Some error happened, check sline_err for reason. */
	} else {
		/* Process input stored in buf */
	}
	
	/* Restore terminal back to normal */
	sline_end();
```

For more complex use cases, you may have a look at projects like:

* [c2f](https://github.com/ariadnavigo/c2f)
* [cras](https://github.com/ariadnavigo/cras)
* [scalc](https://github.com/ariadnavigo/scalc)

Other procedures in sline allow for checking errors, retrieving user input 
history, setting the default prompt symbol, etc. You may check the 
``sline(3)`` and all related manpages cited under its ``SEE ALSO`` section for 
further usage information.  

## Build

sline is supported on Linux and OpenBSD, and requires:

1. A C99 compiler

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

## License

sline is published under an MIT/X11/Expat-type License. See ``LICENSE`` file
for copyright and license details.

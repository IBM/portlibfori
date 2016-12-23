# Summary
Provides various functions for PASE which are not provided by the AIX runtime.

This makes porting applications easier. Currently provides:

- libutil - openpty/forkpty/login_tty APIs and getopt_long

# Compiling

    make
    make install

The Makefile supports PREFIX and DESTDIR variables. The default prefix is /usr/local. To install to a different prefix specify it like so:

    make PREFIX=/QOpenSys/my/prefix install

# License
MIT (See file named LICENSE)
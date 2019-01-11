# Porting Library for IBM i

## Summary

Provides various functions for PASE which are not provided by the AIX runtime.

This makes porting applications easier. Currently provides:

- libutil - openpty/forkpty/login_tty APIs and getopt_long

## Building

    make
    make install

The Makefile supports PREFIX and DESTDIR variables. The default prefix is `/usr/local`. To install to a different prefix specify it like so:

    make PREFIX=/QOpenSys/my/prefix install

## License
Most code is licensed under MIT. See [LICENSE](LICENSE) for more info.

# Porting Library for IBM i

## Summary

Provides various functions for PASE which are not provided by the AIX runtime.

This makes porting applications easier. Currently provides the following libraries:

### libutil

Contains various functions found in Linux, BSD, etc but not found on AIX:

- openpty
- forkpty
- login_tty
- getopt_long
- getopt_long_only
- mkdtemp
- backtrace
- backtrace_symbols
- libutil_getprogname
- libutil_setprogname

### libiperf

IBM i performance statistics library, modeled after AIX's libperfstat.

Currently only supports a quite limited selection of APIs:

- iperfstat_memory_get_main_storage_size
- iperfstat_cpu_get_number

## Building

```shell
    make
    make install
```

The Makefile supports `PREFIX` and `DESTDIR` variables. The default prefix is `/usr/local`. To install to a different prefix specify it like so:

```shell
    make PREFIX=/QOpenSys/my/prefix install
```

## License

Most code is licensed under MIT. See [LICENSE](LICENSE) for more info.

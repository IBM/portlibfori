CC=gcc
CFLAGS=-D__PASE__ -fPIC
LDFLAGS=
PREFIX=/usr/local
DESTDIR=

all: build-all

install: install-all

perfstat/libiperf.o: perfstat/iperfstat_cpu.o perfstat/iperfstat_memory.o
	$(CC) -shared $(CFLAGS) $(LDFLAGS) -Wl,-bE:perfstat/libiperf.exp -o $@ $^

perfstat/%.o: perfstat/%.c
	$(CC) -c $(CFLAGS) -Iperfstat -o $@ $^

perfstat/libiperf.imp: perfstat/libiperf.exp
	( \
	echo '#! libiperf.so.1(shr_64.o)'; \
	echo "# 64"; \
	cat perfstat/libiperf.exp; \
	) > perfstat/libiperf.imp

perfstat/libiperf.so.1: perfstat/libiperf.o perfstat/libiperf.imp
	export OBJECT_MODE=32_64
	mkdir -p perfstat/libiperf.tmp
	cp perfstat/libiperf.o perfstat/libiperf.tmp/shr_64.o
	cp perfstat/libiperf.imp perfstat/libiperf.tmp/shr_64.imp
	cd perfstat/libiperf.tmp
	strip -e perfstat/libiperf.tmp/shr_64.o 2> /dev/null || :
	ar -X64 crlo $@ perfstat/libiperf.tmp/*
	rm -r perfstat/libiperf.tmp

perfstat/libiperf.so: perfstat/libiperf.so.1
	ln -sf libiperf.so.1 perfstat/libiperf.so

perfstat/libiperf.target: perfstat/libiperf.so

install-perfstat-libiperf: perfstat/libiperf.so perfstat/libiperf.so.1
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	/QOpenSys/usr/bin/cp -h perfstat/libiperf.so $(DESTDIR)$(PREFIX)/lib
	/QOpenSys/usr/bin/cp -h perfstat/libiperf.so.1 $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp perfstat/libiperf.h $(DESTDIR)$(PREFIX)/include/libiperf.h

util/libutil.o: util/getopt_long.o util/pty.o util/mkdtemp.o util/backtrace.o util/bsd-flock.o util/asprintf.o util/progname.o util/err.o util/isatty.o util/daemon.o
	$(CC) -shared $(CFLAGS) $(LDFLAGS) -Wl,-bE:util/libutil.exp -o $@ $^

util/%.o: util/%.c
	$(CC) -c $(CFLAGS) -Iutil -o $@ $^

util/libutil.imp: util/libutil.exp
	( \
	echo '#! libutil.so.2(shr_64.o)'; \
	echo "# 64"; \
	cat util/libutil.exp; \
	) > util/libutil.imp

util/libutil.so.2: util/libutil.o util/libutil.imp
	export OBJECT_MODE=32_64
	mkdir -p util/libutil.tmp
	cp util/libutil.o util/libutil.tmp/shr_64.o
	cp util/libutil.imp util/libutil.tmp/shr_64.imp
	cd util/libutil.tmp
	strip -e util/libutil.tmp/shr_64.o 2> /dev/null || :
	ar -X64 crlo $@ util/libutil.tmp/*
	rm -r util/libutil.tmp

util/libutil.so: util/libutil.so.2
	ln -sf libutil.so.2 util/libutil.so

util/libutil.target: util/libutil.so

install-util-libutil: util/libutil.so util/libutil.so.2
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	/QOpenSys/usr/bin/cp -h util/libutil.so $(DESTDIR)$(PREFIX)/lib
	/QOpenSys/usr/bin/cp -h util/libutil.so.2 $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include
	mkdir -p $(DESTDIR)$(PREFIX)/include/sys
	cp util/getopt.h $(DESTDIR)$(PREFIX)/include/getopt.h
	cp util/pty.h $(DESTDIR)$(PREFIX)/include/pty.h
	cp util/execinfo.h $(DESTDIR)$(PREFIX)/include/execinfo.h
	cp util/wrapper/file.h $(DESTDIR)$(PREFIX)/include/sys/file.h
	cp util/wrapper/stdio.h $(DESTDIR)$(PREFIX)/include/stdio.h
	cp util/wrapper/unistd.h $(DESTDIR)$(PREFIX)/include/unistd.h
	cp util/wrapper/stdlib.h $(DESTDIR)$(PREFIX)/include/stdlib.h
	cp util/err.h $(DESTDIR)$(PREFIX)/include/err.h

build-all: perfstat/libiperf.target util/libutil.target

install-all: install-perfstat-libiperf install-util-libutil

.PHONY: clean

clean:
	rm -f */*.a */*.o */*.so */*.so.* */*.imp

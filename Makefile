CC=gcc
OBJ=getopt_long.o pty.o
OBJ64=getopt_long_64.o pty_64.o
CFLAGS=-I. -D__PASE__
PREFIX=/usr/local
DESTDIR=

all: libutil.a

install: all
	echo cp *.h $(DESTDIR)$(PREFIX)/include
	echo cp *.a $(DESTDIR)$(PREFIX)/lib

libutil.a: shr.o shr_64.o
	ar -X32_64 crlo $@ $^

shr_64.o: $(OBJ64)
	$(CC) -maix64 $(CFLAGS) $(LDFLAGS) -shared -o $@ $^
	
%_64.o: %.c
	$(CC) -maix64 -c $(CFLAGS) -o $@ $<

shr.o: $(OBJ)
	$(CC) -maix32 $(CFLAGS) $(LDFLAGS) -shared -o $@ $^
	
%.o: %.c
	$(CC) -maix32 -c $(CFLAGS) -o $@ $<

.PHONY: clean

clean:
	rm -f *.a *.o
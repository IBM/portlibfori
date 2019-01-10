OBJ_IPERF=iperfstat_cpu.o iperfstat_memory.o
OBJ64_IPERF=iperfstat_cpu_64.o iperfstat_memory_64.o

libiperf.a: shr_perf.o shr_perf_64.o
	ar -X32_64 crlo $@ $^

shr_perf_64.o: $(OBJ64_IPERF)
	$(CC) -maix64 $(CFLAGS) $(LDFLAGS) $(LDFLAGS64) -shared -o $@ $^

shr_perf.o: $(OBJ_IPERF)
	$(CC) -maix32 $(CFLAGS) $(LDFLAGS) $(LDFLAGS32) -shared -o $@ $^

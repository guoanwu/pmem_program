sfence_cpi:
	g++ cpi_of_sfence.cpp -o sfence_cpi -g -O0 -lpmem

clean:
	rm -rf *.o sfence_cpi

run_sfence:
	perf stat ./sfence_cpi /mnt/pmem0 128 10 1
	perf stat ./sfence_cpi /mnt/pmem0 128 10 0

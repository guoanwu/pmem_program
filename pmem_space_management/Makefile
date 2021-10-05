all:memory_mode_test memkind_dram.so


memory_mode_test:memory_mode_test.o
	gcc -o memory_mode_test memory_mode_test.o -lpthread

memory_mode_test.o:memory_mode_test.c
	gcc -c memory_mode_test.c

memkind_dram.so:memkind_dram.o
	gcc -shared memkind_dram.o -o memkind_dram.so -lmemkind

memkind_dram.o:memkind_dram.c
	gcc -c -fPIC memkind_dram.c

run_memkind:
	time LD_PRELOAD=./memkind_dram.so ./memory_mode_test 1
	time LD_PRELOAD=./memkind_dram.so ./memory_mode_test 2
	time LD_PRELOAD=./memkind_dram.so ./memory_mode_test 4

run_malloc:
	time ./memory_mode_test 1
	time ./memory_mode_test 2
	time ./memory_mode_test 4

clean:
	rm memory_mode_test *.o *.so -rf

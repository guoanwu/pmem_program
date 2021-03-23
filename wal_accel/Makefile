all: wal_acc lib/libaofguard.so lib/libaofguard.a lib/libaofguard_inject.so

wal_acc:lib/libaofguard.so lib/libaofguard_inject.so
	gcc -std=gnu99 wal_acc.c -g -O0 -o wal_acc -L./lib -lpthread -Wall -laofguard -laofguard_inject 

lib/libaofguard.so: src/common.h src/syscall.h src/aofguard.h src/syscall.c src/aofguard.c
	mkdir -p lib
	gcc -std=gnu99 src/syscall.c src/aofguard.c -g -O0 -o lib/libaofguard.so -fPIC -shared -lpthread -Wall

lib/libaofguard.a: src/common.h src/syscall.h src/aofguard.h src/syscall.c src/aofguard.c
	mkdir -p lib
	gcc -std=gnu99 -c src/syscall.c  -g -O0 -o src/syscall.o -Wall
	gcc -std=gnu99 -c src/aofguard.c -g -O0 -o src/aofguard.o -Wall
	ar cr lib/libaofguard.a src/syscall.o src/aofguard.o
	rm -f src/syscall.o
	rm -f src/aofguard.o

lib/libaofguard_inject.so: src/common.h src/syscall.h src/sha1.h src/aofguard.h src/syscall.c src/sha1.c src/aofguard.c src/inject.c
	mkdir -p lib
	gcc -std=gnu99 src/syscall.c src/sha1.c src/aofguard.c src/inject.c -g -O0 -o lib/libaofguard_inject.so -fPIC -shared -lpthread -Wall

clean:
	rm -rf lib wal_acc


run_acc:
	export AOFGUARD_DISABLE_SYNC=yes
	export AOFGUARD_NVM_DIR=/mnt/pmem0
	export AOFGUARD_NVM_SIZE_MB=512
	export LD_LIBRARY_PATH=./lib
	export AOFGUARD_FILENAME_PATTERN=.log
	LD_PRELOAD=./lib/libaofguard_inject.so taskset -c 0 ./wal_acc 

run_org:
	taskset -c 0 ./wal_acc

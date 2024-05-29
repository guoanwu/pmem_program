crc.o: crc.c crc.h
	g++ -c crc.c

log-timestamp.o: log-timestamp.c
	g++ -c log-timestamp.c

hash_kv.o: hash_kv.c hash_kv.h
	g++ -c hash_kv.c

rw_info.o: rw_info.c rw_info.h
	g++ -c rw_info.c -march=native -O0 -g 

lock.o: lock.c lock.h
	g++ -c lock.c

lock_write.o: lock_write.c
	g++ -c  lock_write.c -march=native -O0 -g 

lock_read.o: lock_read.c
	g++ -c lock_read.c -march=native -O0 -g 

lock_write: lock_write.o crc.o rw_info.o log-timestamp.o lock.o 
	g++ -o lock_write lock_write.o crc.o rw_info.o log-timestamp.o lock.o

lock_read: lock_read.o crc.o rw_info.o log-timestamp.o lock.o
	g++ -o lock_read lock_read.o crc.o rw_info.o log-timestamp.o lock.o

lock_service.o: lock_service.c lock.h
	g++ -c lock_service.c

lock_service: lock_service.o hash_kv.o log-timestamp.o
	g++ -o lock_service lock_service.o hash_kv.o log-timestamp.o

dsa_move.o: dsa_move.c dsa_move.h
	g++ -c dsa_move.c -I /usr/inlcude

dsa_meta_write.o: dsa_meta_write.c
	g++ -c dsa_meta_write.c -I /usr/inlcude

dsa_meta_read.o: dsa_meta_read.c
	g++ -c dsa_meta_read.c -I /usr/inlcude

meta_write.o: meta_write.c
	g++ -c meta_write.c -march=native -O0 -g

meta_read.o: meta_read.c
	g++ -c meta_read.c -march=native -O0 -g

meta_write: meta_write.o rw_info.o crc.o
	g++ -o meta_write meta_write.o rw_info.o crc.o

meta_read: meta_read.o rw_info.o crc.o 
	g++ -o meta_read meta_read.o rw_info.o crc.o

dsa_meta_write: dsa_meta_write.o rw_info.o crc.o dsa_move.o
	g++ -o dsa_meta_write dsa_meta_write.o rw_info.o crc.o dsa_move.o -L /usr/lib64 -ldml -lpthread -ldl

dsa_meta_read: dsa_meta_read.o rw_info.o crc.o dsa_move.o
	g++ -o dsa_meta_read dsa_meta_read.o rw_info.o crc.o dsa_move.o -L /usr/lib64 -ldml -lpthread -ldl

sharing_mem.o: sharing_mem.c
	g++ -c sharing_mem.c -O0 -g

sharing_mem: sharing_mem.o rw_info.o crc.o log-timestamp.o
	g++ -o sharing_mem sharing_mem.o rw_info.o crc.o log-timestamp.o

all: lock_service sharing_mem lock_write lock_read lock_service meta_read meta_write dsa_meta_read dsa_meta_write
clean: 
	rm *.o
	rm lock_write lock_read lock_service sharing_mem meta_read meta_write dsa_meta_read dsa_meta_write

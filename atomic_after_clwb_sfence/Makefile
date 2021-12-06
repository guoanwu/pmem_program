CC=gcc
CFLAGS=-std=gnu99 -g -O0
LFLAGS=-lpmem
SRCFILE=reproduce.c
reproduce_program : $(SRCFILE)
	$(CC) $(SRCFILE) -o lock_after_pause -D LOCK_AFTER_PAUSE $(CFLAGS) $(LFLAGS)
	$(CC) $(SRCFILE) -o lock_before_pause -D LOCK_BEFORE_PAUSE $(CFLAGS) $(LFLAGS)
	$(CC) $(SRCFILE) -o lock_without_pause -D LOCK_BEFORE_PAUSE -D LOCK_WITHOUT_PAUSE $(CFLAGS) $(LFLAGS)

clean:
	rm -f lock_after_pause lock_before_pause lock_without_pause perf.data*
 

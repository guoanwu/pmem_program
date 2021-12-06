#
# Makefile for simplekv example
#

PROGS = queue_aof queue_pmemobj
CXXFLAGS = -g -Wall -Werror -std=c++11\
	-DLIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED=1 `pkg-config --cflags valgrind`
LIBS = -lpmemobj

all: $(PROGS)


queue_aof: queue_aof.o
	$(CXX) -o $@ $(CXXFLAGS) $^ $(LIBS)

queue_pmemobj: queue_pmemobj.o
	$(CXX) -o $@ $(CXXFLAGS) $^ $(LIBS)

clean:
	$(RM) *.o

clobber: clean
	$(RM) $(PROGS)

.PHONY: all clean clobber

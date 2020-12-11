rand_namespace:rand_namespace.o
	g++ -o rand_namespace rand_namespace.o -lpthread -lpmem

rand_namespace.o:rand_namespace.cpp
	g++ -c rand_namespace.cpp

clean:
	rm rand_namespace rand_namespace.o -rf

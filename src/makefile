proxy : proxy.o
	gcc -o proxy proxy.o -lpthread

proxy.o : proxy.c DLinkedList.h
	gcc -c proxy.c

clean : 
	rm proxy.o proxy
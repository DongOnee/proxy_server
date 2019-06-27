proxy : proxy.o DLinkedList.o Node.o
	gcc -o proxy proxy.o DLinkedList.o Node.o -lpthread

proxy.o : proxy.c proxy.h
	gcc -c proxy.c

DLinkedList.o : DLinkedList.c DLinkedList.h
	gcc -c DLinkedList.c

Node.o : Node.c Node.h
	gcc -c Node.c

clean : 
	rm proxy *.o
all:
	rm -f bwcontrol.so
	gcc -Wall -fPIC -o bwcontrol.o -c bwcontrol.c  -D_GNU_SOURCE -w -std=c99
	ld -shared -o bwcontrol.so bwcontrol.o -ldl
clean:
	rm -f bwcontrol.o bwcontrol.so

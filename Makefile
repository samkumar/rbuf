# I could separate compilation and linking, but this compiles instantly anyway

all:
	gcc -O0 -ggdb3 rbuf.c main.c -o rbuf_test
	
clean:
	rm -rf *~ *.o rbuf_test

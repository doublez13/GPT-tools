all: libgpt.so

test: tests.run

tests.run: tests.c libgpt.so
	gcc -Wall -o tests.run tests.c libgpt.so

libgpt.so: libgpt.o
	gcc -Wall -shared -fpic -lz -o libgpt.so libgpt.o

libgpt.o: libgpt.c
	gcc -c -Wall -fpic libgpt.c

clean:
	rm *.run *.so *.o

all: libgpt.so

tests: tests.c scrub.c libgpt.so
	gcc -Wall --debug -o bin/tests.run tests.c libgpt.so
	#gcc -Wall --debug -o bin/scrub.run scrub.c libgpt.so

libgpt.so: libgpt.o
	gcc -Wall -shared -fpic -lz -o libgpt.so libgpt.o

libgpt.o: libgpt.c
	gcc -c -Wall -fpic libgpt.c

clean:
	rm *.run *.so *.o

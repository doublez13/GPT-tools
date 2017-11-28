all: libgpt.so

tests: src/tests.c libgpt.so
	mkdir -p build/bin
	gcc -Wall --debug -o build/bin/tests.run src/tests.c build/lib/libgpt.so
	#gcc -Wall --debug -lz -o bin/scrub.run scrub.c libgpt.so

libgpt.so: libgpt.o
	mkdir -p build/lib
	gcc -Wall -shared -fpic -lz -luuid -o build/lib/libgpt.so libgpt.o

libgpt.o: src/libgpt.c
	gcc -c -Wall -fpic src/libgpt.c

clean:
	rm -rf build *.o 

all: dinfo

dinfo: dinfo.c
	gcc -Wall --debug dinfo.c -lz -o dinfo.run

clean:
	rm *.run 

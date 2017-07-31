all: dinfo

dinfo: dinfo.c
	gcc -Wall dinfo.c -o dinfo.run

clean:
	rm *.run 

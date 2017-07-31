all: dinfo

dinfo: dinfo.c
	gcc -Wall --debug dinfo.c -o dinfo.run

clean:
	rm *.run 

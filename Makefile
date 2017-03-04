CC=gcc
LD=gcc
CFLAGS=-I. -ggdb -std=c99 -Wall
LDFLAGS= -lm

all: lodepng_code transform

.PHONY: lodepng_code
.PHONY: bmp_code

transform: transform.o pixutils.h pixutils.o lodepng/lodepng.o bmp/bmp.o
	$(LD) -o transform transform.o pixutils.o lodepng/lodepng.o bmp/bmp.o $(LDFLAGS)

transform.o : transform.c
	$(CC) -o transform.o -c $(CFLAGS) transform.c

pixutils.o : pixutils.c
	$(CC) -o pixutils.o -c $(CFLAGS) pixutils.c

lodepng_code:
	$(MAKE) -C lodepng

bmp_code:
	$(MAKE) -C bmp
		
clean:
	$(MAKE) -C lodepng clean	
	$(MAKE) -C bmp clean
	rm *.o
	rm transform

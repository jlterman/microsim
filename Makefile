CC=gcc -g -Wall -pedantic

asm51.o:	intel8051/asm.c
	$(CC) -I ./ intel8051/asm.c -c -o asm51.o

back51.o: 	back.c back.h asm.h front.h
	$(CC) -I intel8051 back.c -c -o back51.o

asm51: 		main.o front.o err.o back51.o asm51.o
	$(CC) main.o front.o err.o back51.o asm51.o -o asm51

test.obj:	asm51 test.asm
	asm51 test.asm

test.out:	test.obj testref.obj
	{ if diff test.obj testref.obj ; then echo asm51 succeeded; else echo asm51 failed ; fi } > test.out

all:		asm51 test.out
	cat test.out

clean:
	\rm -f *.o *.out asm51

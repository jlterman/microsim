CFLAGS=-Wall -pedantic -c
OBJ51=main.o front.o err.o back51.o asm51.o

asm51.o:	intel8051/asm.c intel8051/proc.h
	$(CC) $(CFLAGS) -I ./ intel8051/asm.c -o $@

back51.o: 	back.c back.h asm.h front.h intel8051/proc.h
	$(CC) $(CFLAGS) -I intel8051 back.c -o $@

asm51: 	$(OBJ51)	
	$(CC) $(OBJ51) -o $@

test51.obj:	asm51 intel8051/test.asm
	asm51 intel8051/test.asm

test51.out:	test51.obj intel8051/testref.obj
	@{ if diff intel8051/test.obj intel8051/testref.obj ; then echo asm51 succeeded; else echo asm51 failed ; fi } > test51.out ; cat test51.out

asm6502.o:	m6502/asm.c
	$(CC) $(CFLAGS) -I ./ m6502/asm.c -c -o asm6502.o

back6502.o:	back.c back.h asm.h front.h
	$(CC) -I m6502 back.c -c -o back6502.o

asm6502:	main.o front.o err.o back6502.o asm6502.o
	$(CC) main.o front.o err.o back6502.o asm51.o -o asm6502

all:		asm51 test51.out asm6502 
	cat test51.out test6502.out

clean:
	\rm -f *.o* *.out asm51 asm6502

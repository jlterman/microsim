CFLAGS=-Wall -pedantic -c -I ./ -I ./include
OBJS=main.o front.o err.o

version.h: asm_vers *.c
	echo \#define ASM_VERS \"version `cat asm_vers`, build date\: `date "+%B %d, %Y %k:%M:%S"`\">version.h; \rm -f main.o

back51:
	cd intel8051; make all

asm51: 	back51 version.h $(OBJS) intel8051/back.o intel8051/asm.o
	$(CC) $(OBJS) intel8051/back.o intel8051/asm.o -o $@

test:	asm51
	cd intel8051; make test

all:	test

clean:
	\rm -f version.h *.o asm51 ; cd ./intel8051 ; make clean 

CFLAGS=-O -Wall -pedantic -c -I ./ -I ../include
OBJS=back.o asm.o sim.o

version.h: cpu_vers asm.c proc.h
	echo \#define CPU_VERS \"version `cat cpu_vers`, build date\: `date "+%B %d, %Y %k:%M:%S"`\">version.h; \rm -f asm.o

back.o:	../back.c proc.h
	$(CC) $(CFLAGS) ../back.c -o $@

all:	version.h $(OBJS)

test.obj:	../asm51 test.asm
	../asm51 test.asm -v

test:	test.obj
	@{ if diff test.obj testref.obj ; then echo asm51 succeeded; else echo asm51 failed ; fi } > test.out ; cat test.out

clean:
	\rm -f *.o *.out version.h

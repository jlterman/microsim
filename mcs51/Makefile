CFLAGS=-O -Wall -pedantic -c -I ./ -I ../include
OBJS=back.o asm.o sim.o

version.h: cpu_vers asm.c proc.h
	echo \#define CPU_VERS \"version `cat cpu_vers`, build date\: `date "+%B %d, %Y %k:%M:%S"`\">version.h; \rm -f asm.o

back.o:	../back.c proc.h
	$(CC) $(CFLAGS) ../back.c -o $@

all:	version.h $(OBJS)

reset_test:
	\rm -f *.obj *.out

%.obj: %.asm ;	../asm51 $<

%.out: %.obj;	@{ if diff $< $(<).ref ; then echo asm51 succeeded; else echo asm51 failed ; fi } > $@ ; cat $@

test:	reset_test testrom.out testreg.out testall.out
	@if grep -s fail *.out ; then echo test failed!!! ; else echo test okay ; fi ; \rm -f *.obj

clean:
	\rm -f *.o *.obj *.out version.h

CFLAGS=-Wall -pedantic -c -I ./ -I ../include
VPATH=../
ASM6502_OBJS=asm.o proc.o
SIM6502_OBJS=sim.o

version_6502.h: cpu_vers $(addsuffix .c,$(basename $(ASM6502_OBJS) $(SIM6502_OBJS)))
	echo \#define CPU_VERS \"version `cat cpu_vers`, build date\: `date "+%B %d, %Y %k:%M:%S"`\">version.h; \rm -f asm.o

asm6502: main.o $(ASM_OBJS) $(ASM6502_OBJS)
	$(CC) $^ -o $@

sim6502: main_sim.o $(SIM_OBJS) $(ASM_OBJS) $(ASM6502_OBJS) $(SIM6502_OBJS)
	$(CC) $^ -o $@

proc.o: proc.c
	$(CC) $(CFLAGS) $^ -o proc.o

reset_test:
	\rm -f *.obj *.out

%.obj: %.asm ;	../asm6502 $<

%.out: %.obj;	@{ if diff $< $(<).ref ; then echo asm6502 succeeded; else echo asm6502 failed ; fi } > $@ ; cat $@

test:	
	@echo no test for 6502

clean:
	\rm -f *.o *.obj *.out version_6502.h asm6502 sim6502

all:	version_6502.h asm6502 sim6502 test

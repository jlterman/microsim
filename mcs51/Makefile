CFLAGS=-Wall -pedantic -c -I ./ -I ../include
VPATH=../
ASM51_OBJS=asm.o proc.o
SIM51_OBJS=sim.o

version_mc51.h: cpu_vers $(addsuffix .c,$(basename $(ASM51_OBJS) $(SIM51_OBJS)))
	echo \#define CPU_VERS \"version `cat cpu_vers`, build date\: `date "+%B %d, %Y %k:%M:%S"`\">$@; \rm -f asm.o

asm51: main.o $(ASM_OBJS) $(ASM51_OBJS)
	$(CC) $^ -o $@ ; $@ -V

sim51: main_sim.o $(SIM_OBJS) $(ASM_OBJS) $(ASM51_OBJS) $(SIM51_OBJS)
	$(CC) $^ -o $@ ; $@ -V

proc.o: proc.c
	$(CC) $(CFLAGS) $^ -o proc.o

clean:
	\rm -f *.o *.obj *.out version_mc51.h asm51 sim51

all:	version_mc51.h asm51 sim51
	cd regtest; $(MAKE) all

CFLAGS=-Wall -pedantic -c -I ./ -I ../include
VPATH=../
ASM_OBJS=asm.o proc.o sim.o
export PROC=mc51

version_cpu.h: cpu_vers $(addsuffix .c,$(basename $(ASM_OBJS)))
	echo \#define CPU_VERS \"version `cat cpu_vers`, build date\: `date "+%B %d, %Y %k:%M:%S"`\">$@; \rm -f asm.o

asm: main.o $(ASM_OBJS) $(OBJS)
	$(CC) $^ -o $@ ; $@ -V

sim: asm
	cp asm sim

proc.o: proc.c
	$(CC) $(CFLAGS) $^ -o proc.o

clean:
	\rm -f *.o version_cpu.h asm asm sim
	$(MAKE) -C regtest clean

all:	version_cpu.h asm sim
	cd regtest; $(MAKE) all


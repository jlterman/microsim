CFLAGS=-Wall -pedantic -c -I ./ -I ./include
export ASM_OBJS=expr.o front.o back.o
export SIM_OBJS=sim_run.o
export OBJS=main.o main_sim.o $(ASM_OBJS) $(SIM_OBJS)

version.h: asm_vers *.c
	echo \#define ASM_VERS \"version `cat asm_vers`, build date\: `date "+%B %d, %Y %k:%M:%S"`\">version.h; \rm -f main.o

mc51:
	cd intel8051; $(MAKE) all 

mc51_clean:
	cd intel8051; $(MAKE) clean

mot6502:
	cd m6502; $(MAKE) all

mot6502_clean:
	cd m6502; $(MAKE) clean

all:	version.h $(OBJS) mc51 mot6502

clean:  mc51_clean mot6502_clean
	\rm -f version.h *.o

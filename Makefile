CFLAGS=-Wall -pedantic -c -I ./ -I ./include
TARGS=$(addsuffix .trg, $(dir $(wildcard */Makefile)))
export OBJS=main.o expr.o front.o back.o sim_run.o

version.h: sim_vers asm_vers *.c
	echo \#define ASM_VERS \"version `cat asm_vers`\" > $@
	echo \#define SIM_VERS \"version `cat sim_vers`\" >> $@
	echo \#define BUILD_DATE \"`date "+%B %d, %Y %k:%M:%S"`\" >> $@
	\rm -f main*.o

%.trg:
	$(MAKE) -C $* $(TRGCMD)

build:	version.h $(OBJS) $(TARGS)
	$(MAKE) $(TARGS) TRGCMD=build

test:	build
	$(MAKE) $(TARGS) TRGCMD=test

clean:  
	\rm -f version.h *.o
	$(MAKE) $(TARGS) TRGCMD=clean

all:	clean build test

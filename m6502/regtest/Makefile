ASM_TESTS=$(addsuffix .obj.out, $(basename $(wildcard *.asm)))
SIM_TESTS=$(addsuffix .run.out, $(basename $(wildcard *.sim)))

clean:
	\rm -f *.run *.obj *.out asm?????? sim??????

%.obj: %.asm ;	  ../asm $<

%.obj.out: %.obj; @{ if diff $< $(<).ref ; then echo $< succeeded; else echo $@:1 '******' $@ failed ; fi } > $@ ; cat $@

%.run: %.sim; ../sim $*.asm < $< > $@

%.run.out: %.run; @{ if diff $< $(<).ref ; then echo $< succeeded; else echo $@:1 '******' $@ failed ; fi } > $@ ; cat $@

all:	clean $(ASM_TESTS) $(SIM_TESTS)
	@echo Running $(PROC) assembly and simulator tests
	@if grep -n fail *.out ; then echo test failed!!! ; else echo test okay ; fi

dac     equ     0800h   ; D/A converter device addr
bstore	equ	20h

pstore  equ  80h
        mov  pstore + 2, #0
        orl  c, /P2.3
        mov  ((256*40 + 25)/50), r0
        mov  (25/8) + 2*(25%8), c  ;  bitaddr equ 25/8 + (25%8) 
        mov  (25/8) + 25%8, c  ;  bitaddr equ 25/8 + (25%8) 
        addc a, #-5
        jbc  p1.0, addr40
        jbc  80h, addr40
        ajmp 7ffh
        anl  c, (25 / 8) + 25 %8
        jnb  90h+5, addr56
        mov  dptr, #dac
addr40: cjne a, #14, addr56
        mov  85, a
        mov  r7, a
        setb 80h
        setb P1.0
        dw   -1023
        db   -5
        db   'a'
        mov  r0, #16
        db   0c0h    ; 0
        inc  r1
addr56: mov  r7, a
        ret
	jnb  bstore.0, addr40
	jb   20h.0, addr40
	jbc  20h, addr40

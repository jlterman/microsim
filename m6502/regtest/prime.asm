pstore	    equ	300h
bitd	    equ	0
quotient    equ 1
dividend    equ 2
divisor	    equ 3

	org 200h

prime:	lda #2
	sta pstore
	lda #3
	sta pstore + 1		;  seed array with first 2 primes
	lda #5
	sta pstore + 2		; last array value test prime
	ldx #2			; number of primes and index of test prime
checkp:	ldy #1			; start at 2nd prime (skip 2)
divp:	lda pstore, x
	sta dividend
	lda pstore, y
	sta divisor
	jsr div8		; divide test prime by prime factor
	lda dividend		; yes, look at remainder
	beq nextp		; if remainder zero, this number is not prime
	lda pstore, y
	cmp quotient		; was test prime/prime factor <= prime factor
	bpl found		; if yes, number is prime
	iny			; next prime factor
	bne divp		; divide next prime factor
found:	lda pstore, x		; get new prime
	inx			; advance to new prime location
	sta pstore, x		; put last prime+2 for next prime candidate
nextp:	inc pstore, x
	inc pstore, x		;  new prime candidate
	lda pstore, x
	cmp #0ffh		; don't go past 255
	bmi checkp
	txa			; put number of prime in accumulator
done:	rts

div8:	lda #1			; start dividing pstore,x/pstore,y
	sta bitd		; start at bit 1
	lda #0
	sta quotient		; clear quotient
	lda divisor		; get divisor in a
@1:	cmp dividend		; compare to dividend
	bpl startd		; if shift divisor>dividend, start div
	asl bitd
	asl a			; shift bit and divisor left
	bpl @1			; until divisor>dividend or divisor>7fh
startd:	sta divisor		; save divisor
@1:	lda dividend
	cmp divisor		; dividend > divisor
	bmi nxtbit		; no, don't subtract, but get next bit
	lda dividend
	sec
	sbc divisor		; subtract divisor from dividend
	sta dividend
	lda quotient
	ora bitd		; if we subtrct divisor, set corresponding
	sta quotient		; bit of quotient
nxtbit:	lsr divisor		; next rightmost bit
	lsr bitd
	bne @1			; done all bit? if no, continue
	rts
	
	org 0FFFCh
	db  prime%256, prime/256

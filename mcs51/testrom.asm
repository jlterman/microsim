	; TSH ROM Listing
	; Todd Sauke and Jim Terman
	; Last modified 9/15/2003

	dac	equ	0800h	; D/A converter device addr
	led0	equ	1000h	; LED0 
	led1	equ	1800h	; LED1
	dip	equ	2000h	; Dip switch
	adc	equ	2800h	; A/D converter	

	sega	equ	0feh	; segment mapping
	segb	equ	0fdh
	segc	equ	0fbh
	segd	equ	0f7h
	sege	equ	0efh
	segf	equ	0dfh
	segg	equ	0bfh

	let_r	equ	0afh	; letter mapping
	let_a	equ	0a0h	
	let_n	equ	0abh
	let_d	equ	0a1h
	let_y	equ	091h
	blank	equ	0ffh


		org	0	; ROM location 0
		ajmp	test	; jump on reset

		org	20h	; leave room for interrupts
	test:	mov	r0, #16	; run through all the hex digits
		mov	r1, #0	; for startup test
		mov	a, r1

	test1:	lcall	digmap
		lcall	dsplay
		lcall	wait
		inc	r1
		mov	a, r1
		djnz	r0, test1
	
	scan:	mov	dptr, #dip	; scan dip
		movx	a, @dptr
		anl	a, #0fh	; ignore 4 highest bits
		lcall	jmpsub	; call nth subroutine
		sjmp	scan
		
	digmap:	inc		a	; digit mapping
		movc	a, @a+pc	; get digit
		ret
		db	0c0h	; 0
		db	0f9h	; 1
		db	0a4h	; 2
		db	0b0h	; 3
		db	99h	; 4
		db	92h	; 5
		db	82h	; 6
		db	0f8h	; 7
		db	80h	; 8
		db	90h	; 9
		db	88h	; A
		db	83h	; b
		db	0c6h	; C
		db	0a1h	; d
		db	86h	; E
		db	8eh	; F

	dsplay:	mov	dptr, #led0	; write to LEDs
		movx	@dptr, a
		mov	dptr, #led1
		movx	@dptr, a
		ret

	dsply0:	mov	dptr, #led0
		movx	@dptr, a
		ret

	dsply1:	mov	dptr, #led1
		movx	@dptr, a
		ret

	dsphex:	mov	B, #16
		div	ab
		lcall	digmap
		lcall	dsply1
		mov	A, B
		lcall	digmap
		lcall	dsply0
		ret

	wait:	mov	r6, #0	; wait loop
		mov	r7, #0
	wait1:	djnz	r6, wait1
		djnz	r7, wait1
		ret

	wait4:	lcall	wait
		lcall	wait
		lcall	wait
		lcall	wait
		ret

	jmpsub:	rl	a	; scale for jmp table
		mov	dptr, #jmptbl
		jmp	@a+dptr	; jump to function

	jmptbl:	ajmp	dip0	; display "randy"
		ajmp	dip1	; moving segment display
		ajmp	dipled	; jump when dip is 2
		ajmp	dipled	; jump when dip is 3
		ajmp	dipled	; jump when dip is 4
		ajmp	dipled	; jump when dip is 5
		ajmp	dipled	; jump when dip is 6
		ajmp	dipled	; jump when dip is 7
		ajmp	dipled	; jump when dip is 8
		ajmp	dipled	; jump when dip is 9
		ajmp	dipled	; jump when dip is 10
		ajmp	dipled	; jump when dip is 11
		ajmp	dipled	; jump when dip is 12
		ajmp	dipled	; jump when dip is 13
		ajmp	dipled	; jump when dip is 14
		ajmp	dipled	; jump when dip is 15

	dipled:	rr	a	; scale back a and put on LED
		lcall	dsplay
		ret
	
	dip0:	mov	a, #let_r	; display randy
		lcall	dsplay
		lcall	wait4
		mov	a, #let_a
		lcall	dsplay
		lcall	wait4
		mov	a, #let_n
		lcall	dsplay
		lcall	wait4
		mov	a, #let_d
		lcall	dsplay
		lcall	wait4
		mov	a, #let_y
		lcall	dsplay
		lcall	wait4
		mov	a, #blank
		lcall	dsplay
		lcall	wait4
	ret
		
	dip1:	mov	a, #sega	; moving segments
		mov	r0, #07h	; loop 7 times
	dip1a:	lcall	dsplay
		lcall	wait
		rl	a		; shift bits
		djnz	r0, dip1a
		ret

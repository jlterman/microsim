	pstore equ 20h		; array of primes

prime:	mov	pstore, #2
	mov	pstore+1, #3	; seed prime array with first 2 primes
	mov	r2, #5		; next value to test
	mov	r0, #pstore+2	; r0 = addr of next prime
checkp:	mov	r1, #pstore+1	; r1 = addr of first prime (skip 2)
divp:	mov	b, @r1		; divide test prime by prime factor
	mov	a, r2
	div	ab		; get testp/pstore[p]
	xch	a, b
	jz	nextp		; if quotient is zero, try next number
	mov	a, @r1
	cjne	a, b, @1	; compare quotient with pstore[p]
@1:	jnc	found		; if pstore[p]>=quotient prime is found
	inc	r1		; could be prime, try next prime factor
	sjmp	divp
found:	mov	a, r2		; prime found
	mov	@r0, a		; store in pstore
	inc	r0		; adv index to end of pstore
nextp:	inc	r2
	inc	r2		; next odd test prime
	cjne	r2, #0FFh, checkp ; check for all primes<255
	mov	a, r0
	add	a, #-pstore	; return with number of primes in acc
done:	ret
	

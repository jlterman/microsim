pstore	equ 30h

addr0:  ora ( pstore + 2 , x )
        ora 3*(pstore + 2)
        ora 256*(pstore + 2)
        ora #-5
        ora 2*(0ffh+3) , y
        ora 2*(25%8)+(256*40 + 25)/50, x
	beq addr0
	bcc addr1
        dw   -1023
        db   -5
        db   'a'
	db 5, 6, 7, 8, 9, 10, 11
addr1:	rts

        nop
        ajmp 0
        ljmp addr30
        rr a
        inc a
        inc 20h
        inc @r0
        inc @r1
addr00: inc r0
        inc r1
        inc r2
        inc r3
        inc r4
        inc r5
        inc r6
        inc r7
addr01: jbc 80h, addr02
        acall 0
        lcall addr30
        rrc a
        dec a
        dec 30h
        dec @r0
        dec @r1
addr02: dec r0
        dec r1
        dec r2
        dec r3
        dec r4
        dec r5
        dec r6
        dec r7
addr03: jb P1.1, addr05
        ajmp 100h
        ret
        rl a
        add a, #0FFh
        add a, 31h
        add a, @r0
        add a, @r1
addr04: add a, r0
        add a, r1
        add a, r2
        add a, r3
        add a, r4
        add a, r5
        add a, r6
        add a, r7
addr05: jnb 90h+5, addr08
        acall 100h
        reti
        rlc a
        addc a, #5
        addc a, 41h
        addc a, @r0
        addc a, @r1
addr06: addc a, r0
        addc a, r1
        addc a, r2
        addc a, r3
        addc a, r4
        addc a, r5
        addc a, r6
        addc a, r7
addr07: jc addr11
        ajmp 200h
        orl 51h, a
        orl 129, #2+(8/5)
        orl a, #(256*4/5)
        orl a, 15
        orl a, @r0
        orl a, @r1
addr08: orl a, r0
        orl a, r1
        orl a, r2
        orl a, r3
        orl a, r4
        orl a, r5
        orl a, r6
        orl a, r7
addr09: jnc addr16
        acall 200h
        anl 18, a
        anl 19, #20
        anl a, #255
        anl a, 20
        anl a, @r0
        anl a, @r1
addr10: anl a, r0
        anl a, r1
        anl a, r2
        anl a, r3
        anl a, r4
        anl a, r5
        anl a, r6
        anl a, r7
addr11: jz addr08
        ajmp 300h
        xrl 23, a
        xrl 24, #1
        xrl a, #255-80h
        xrl a, 25
        xrl a, @r0
        xrl a, @r1
addr12: xrl a, r0
        xrl a, r1
        xrl a, r2
        xrl a, r3
        xrl a, r4
        xrl a, r5
        xrl a, r6
        xrl a, r7
addr13: jnz addr17
        acall 300h
        orl c, 85h
        jmp @a+dptr
        mov a, #0
        mov 28, #1
        mov @r0, #2
        mov @r1, #3
addr14: mov r0, #4
        mov r1, #5
        mov r2, #6
        mov r3, #7
        mov r4, #8
        mov r5, #9
        mov r6, #10
        mov r7, #11
addr15: sjmp addr10
        ajmp 400h
        anl c, (25/8)
        movc a,@a+pc
        div ab
        mov 30, 20
        mov 32, @r0
        mov 33, @r1
addr16: mov 34, r0
        mov 35, r1
        mov 36, r2
        mov 37, r3
        mov 38, r4
        mov 39, r5
        mov 40, r6
        mov 41, r7
addr17: mov dptr, #12
        acall 400h
        mov psw.2, C
        movc a, @a+dptr
        subb a, #13
        subb a, addr20
        subb a, @r0
        subb a, @r1
addr18: subb a, r0
        subb a, r1
        subb a, r2
        subb a, r3
        subb a, r4
        subb a, r5
        subb a, r6
        subb a, r7
addr19: orl c, /P2.3
        ajmp 500h
        mov c, 36
        inc dptr
        mul ab
        db 0a5h      ;  reserved op
        mov @r0, 45
        mov @r1, 46
addr20: mov r0, 47
        mov r1, 48
        mov r2, 49
        mov r3, 50
        mov r4, 51
        mov r5, 52
        mov r6, 53
        mov r7, 54
addr21: anl c, /acc.7
        acall 500h
        cpl b.7
        cpl c
        cjne a, #14, addr26
        cjne a, 16, addr27
        cjne @r0, #15, addr28
        cjne @r1, #16, addr29
addr22: cjne r0, #17, addr20
        cjne r1, #18, addr21
        cjne r2, #19, addr22
        cjne r3, #20, addr23
        cjne r4, #21, addr24
        cjne r5, #22, addr25
        cjne r6, #23, addr26
        cjne r7, #24, addr17
addr23: push 68
        ajmp 600h
        clr acc.7
        clr c
        swap a
        xch a, 70
        xch a, @r0
        xch a, @r1
addr24: xch a, r0
        xch a, r1
        xch a, r2
        xch a, r3
        xch a, r4
        xch a, r5
        xch a, r6
        xch a, r7
addr25: pop 71
        acall 600h
        setb 25
        setb c
        da a
        djnz 50h, addr23
        xchd a, @r0
        xchd a, @r1
addr26: djnz r0, addr24
        djnz r1, addr25
        djnz r2, addr26
        djnz r3, addr27
        djnz r4, addr28
        djnz r5, addr29
        djnz r6, addr30
        djnz r7, addr21
addr27: movx a, @dptr
        ajmp 700h
        movx a, @r0
        movx a, @r1
        clr a
        mov a, 83
        mov a, @r0
        mov a, @r1
addr28: mov a, r0
        mov a, r1
        mov a, r2
        mov a, r3
        mov a, r4
        mov a, r5
        mov a, r6
        mov a, r7
addr29: movx @dptr, a
        acall 700h
        movx @r0, a
        movx @r1, a
        cpl a
        mov 85, a
        mov @r0, a
        mov @r1, a
addr30: mov r0, a
        mov r1, a
        mov r2, a
        mov r3, a
        mov r4, a
        mov r5, a
        mov r6, a
        mov r7, a

addr0:  brk
        ora ( 0 , x )
        ora 20h
        asl 64
        php
        ora #-5
        asl a
        ora 1000h
        asl 32768
        bpl addr0
        ora ( 0ffh ) , y
        ora 0E0h , x
        asl 80h , x
        clc
        ora 2000h , y
        ora 2000h , x
        asl 2000h , x
        jsr addr3
        and ( 10h , x )
        bit 20
        and 30
        rol 40
        plp
        and #0FFh
        rol a
        bit 0F000h
        and 0E000h
        rol 0D000h
        bmi addr1
        and ( 0 ) , y
        and ( 1 ) , x
        rol ( 2 ) , x
        sec
        and 0C000h , y
        and 0B000h , x
        rol 0a000h , y
        rti
        eor ( 50 , x )
        eor 60
        lsr 70
        pha
        eor #80
        lsr a
        jmp 0FFFFh
        eor 0FFFFh
        lsr 0
        bvc addr3
        eor ( 40h ) , y
        eor 40h , x
        lsr 40h , x
        cli
addr3:	eor 0FFFFh , y
        eor 0FFFFh , x
        lsr 0FFFFh , x
        rts
        adc ( 40h , x )
        adc 40h
        ror 40h
        pla
        adc #-127
        ror a
        jmp ( 0FFFFh )
        adc 0FFFFh
addr1:  ror 0FFFFh
        bvs addr1
        adc ( 40h )
        adc 40h , x
        adc 40h , x
        sei
        adc 0FFFFh , y
        adc 0FFFFh , x
        ror 0FFFFh , x
        sta ( 40h , x )
        sty 40h
        sta 40h
        stx 40h
        dey
        txa
        sty 0FFFFh
        sta 0FFFFh
        stx 0FFFFh
        bcc addr2
        sta ( 40h ) , y
        sty 40h , x
        sta 40h , x
        stx 40h , y
        tya
        sta 0FFFFh , y
        txs
        sta 0FFFFh , x
        ldy #2
        lda ( 40h , x )
        ldx #50
        ldy 40h
        lda 40h
        ldx 40h
        tay
        lda #60
        tax
        ldy 0FFFFh
        lda 0FFFFh
        ldx 0FFFFh
        bcs addr3
        lda ( 40h ) , y
        ldy 40h , x
        lda 40h , x
        ldx 40h , y
        clv
        lda 0FFFFh , y
        tsx
        ldy 0FFFFh , x
        lda 0FFFFh , x
        ldx 0FFFFh , y
        cpy #70
        cmp ( 40h , x )
        cpy 40h
        cmp 40h
        dec 40h
        iny
        cmp #80
        dex
        cpy 0FFFFh
        cmp 0FFFFh
        dec 0FFFFh
        bne addr2
        cmp ( 40h ) , y
        cmp 40h , x
addr2:  dec 40h , x
        cld
        cmp 0FFFFh , y
        cmp 0FFFFh , x
        dec 0FFFFh , x
        cpx #90
        sbc ( 40h , x )
        cpx 40h
        sbc 40h
        inc 40h
        inx
        sbc 20h
        nop
        cpx 0FFFFh
        sbc 0FFFFh
        inc 0FFFFh
        beq addr4
        sbc ( 40h ) , y
        sbc 40h , x
        inc 40h , x
        sed
        sbc 0FFFFh , y
        sbc 0FFFFh , x
addr4:  inc 0FFFFh , x

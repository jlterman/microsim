/*************************************************************************************

    Copyright (c) 2003, 2004 by James L. Terman
    This file is part of the 6502 Assembler backend

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *************************************************************************************/
#ifndef _PROC_HEADER
#define _PROC_HEADER

#include "front.h"

/*
 *
 *  6502 Processor Specific Definitions
 *
 */

#define ENDIAN LITTLE

/* enum instr_tokens are the index values + 1 for the token array above plus
 * extra tokens which describe the parameters of the 6502 instructions set.
 */
enum instr_tokens 
  { 
    pound = PROC_TOKEN, leftPar, rightPar, a, adc, add, and, asl,
    bcc, bcs,  beq, bit, bmi, bne, bpl, brk,
    bvc, bvs,  clc, cld, cli, clv, cmp, cpx,
    cpy, dec,  dex, dey, eor, inc, inx, iny,
    jmp, jsr,  lda, ldx, ldy, lsr, nop, NOP,
    ora, p, pc_reg, pha, php, pla, plp, rol, 
    ror, rti,  rts, sbc, sec, sed, sei,  sp, 
    sta, stx,  sty, tax, tay, tsx, txa, txs, 
    tya, x,    y,   LAST_PROC_TOKEN
  };

#ifndef CPU_LOCAL
extern const int isRegister_table[LAST_PROC_TOKEN - PROC_TOKEN];
#endif

#define isRegToken(x) isRegister_table[(int) x]

#endif

/*************************************************************************************

    Copyright (c) 2003 by Jim Terman
    This file is part of the 8051 Assembler

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
 *  8051 Processor Specific Definitions
 *
 */

#define ENDIAN BIG

/* enum instr_tokens are the index values for the tokens array defined in 
 * asm.c plus extra tokens which describe the parameters of the 8051 instructions set.
 */
enum instr_tokens 
  { 
    pound = PROC_TOKEN, dot, slash, a_dptr, a_pc, at_dptr, at_r0, at_r1,
    a, ab, acall, add, addc, ajmp, anl, c, 
    cjne, clr, cpl, da, dec, divab, djnz, dptr, 
    inc, jb, jbc, jc, jmp, jnb, jnc, jnz, 
    jz, lcall, ljmp, mov, movc, movx, mul, nop, 
    orl, pc_reg, pop, push, r0, r1, r2, r3, 
    r4, r5, r6, r7, ret, reti, rl, rlc, 
    rr, rrc, setb, sjmp, subb, swap, xch, xchd, 
    xrl, LAST_PROC_TOKEN, rsrvd, CONST_PROC_TOKEN, 
    addr_11, bit_addr, bit_byte, bit_no, LAST_TOKEN
  };

#define P0     0x80 
#define SP     0x81 
#define DPL    0x82 
#define DPH    0x83 
#define PCON   0x87
#define TCON   0x88 
#define TMOD   0x89 
#define TL0    0x8A 
#define TL1    0x8B 
#define TH0    0x8C 
#define TH1    0x8D 
#define P1     0x90 
#define SCON   0x98 
#define SBUF   0x99 
#define P2     0xA0
#define IE     0xA8
#define P3     0xB0 
#define IP     0xB8
#define T2CON  0xC8 
#define RCAP2L 0xCA
#define RCAP2H 0xCB 
#define TL2    0xCC 
#define TH2    0xCD 
#define PARITY 0xD0
#define PSW    0xD0 
#define USRFLG 0xD1
#define OV     0xD2
#define RS1    0xD4
#define RS2    0xD5
#define FLAG0  0xD5
#define AUXC   0xD6
#define CARRY  0xD7
#define ACC    0xE0
#define B      0xF0 

#endif

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

/*
 *
 *  8051 Processor Specific Definitions
 *
 */

#define ENDIAN BIG

/* TOKEN_LENGTH and TOKEN bit are used for binary search of this array.
 */
#define TOKEN_LENGTH 67
#define TOKEN_BIT    6

/* enum instr_tokens are the index values + 1 for the token array above plus
 * extra tokens which describe the parameters of the 8051 instructions set.
 */
enum instr_tokens 
  { 
    pound = 1, dot, slash, a_dptr, a_pc, at_dptr, at_r0, at_r1,
    a, ab, acall, add, addc, ajmp, anl, c,
    cjne, clr, cpl, da, dec, divab, djnz, dptr,
    inc, jb, jbc, jc, jmp, jnb, jnc, jnz,
    jz, lcall, ljmp, mov, movc, movx, mul, nop,
    orl, pop, push, r0, r1, r2, r3, r4,
    r5, r6, r7, ret, reti, rl, rlc, rr,
    rrc, setb, sjmp, subb, swap, xch, xchd, xrl,
    aj_dup, ac_dup, rsrvd, last_tok, 
    addr_11 = 768, bit_addr, bit_byte, bit_no
  };

/* The macro isInstr distinguishes tokens that represent 8051 instructions
 * and those that are tokens for the parameters of 8051 instructions.
 * includes psuedo-ops
 */
#define isInstr(x) ((x<last_tok)?isInstr_table[x-1]:0)

#ifndef CPU_LOCAL
extern 
#endif
int handleInstr(const int*, int*);

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
#define CPU_LOCAL

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "asm.h"
#include "err.h"
#include "front.h"
#include "back.h"
#include "proc.h"

/* The token array contains all the reserved words for 8051 assembly. 
 */
const str_storage tokens[] = 
  { 
    "#", ",", ".", "/", "@a+dptr", "@a+pc", "@dptr", "@r0", "@r1", "a", "ab", "acall", 
    "add", "addc", "ajmp", "anl", "c", "cjne", "clr", "cpl", "da", "db", "dec", "div",
    "djnz", "dptr", "dw", "inc", "jb", "jbc", "jc", "jmp", "jnb", "jnc", "jnz", 
    "jz", "lcall", "ljmp", "mov", "movc", "movx", "mul", "nop", "orl", "pop", 
    "push", "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "ret", "reti", "rl", 
    "rlc", "rr", "rrc", "setb", "sjmp", "subb", "swap", "xch", "xchd",  "xrl", 0
  };

/* Macro returns true when character is a 1 char token
 * Note: no intersection with isToken(x)
 */
const int isCharToken_table[ASCII_MAX] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, /*  !"#$%&'()*+,-./ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0123456789:;<=>? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* @ABCDEFGHIJKLMNO */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* PQRSTUVWXYZ[\]^_ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* `abcdefghijklmno */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* pqrstuvwxyz{|}~  */
  };

/* Macro returns true for legal chars that make up a 
 * multi-char token or an expression.
 */
const int isToken_table[ASCII_MAX] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, /*  !"#$%&'()*+,-./ */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 0123456789:;<=>? */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* @ABCDEFGHIJKLMNO */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, /* PQRSTUVWXYZ[\]^_ */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* `abcdefghijklmno */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0  /* pqrstuvwxyz{|}~  */
  };

/* isInstr_table distinguishes tokens that represent 8051 instructions
 * and those that are tokens for the parameters of 8051 instructions.
 * includes psuedo-ops
 */
const int isInstr_table[last_tok] = 
  { 
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 1, 1, 1, 1, 1, 
    0, 1, 1, 1, 1,-1, 1, 1, 
    1, 0,-1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 0, 0, 
    0, 0, 0, 0, 0, 0, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 0, 
  };

/* This array contains all the parameter info 
 * for every instruction in the 8051.
 */
const int cpu_instr_tkn[][INSTR_TKN_BUF] = 
{
  { 0x00, 1, 1, nop, 0 },
  { 0x01, 2, 2, ajmp,  addr_11,  0 },
  { 0x02, 3, 2, ljmp,  addr_16,  0 },
  { 0x03, 1, 1, rr,    a,        0 },
  { 0x04, 1, 1, inc,   a,        0 },
  { 0x05, 2, 1, inc,   addr_8,   0 },
  { 0x06, 1, 1, inc,   at_r0,    0 },
  { 0x07, 1, 1, inc,   at_r1,    0 },
  { 0x08, 1, 1, inc,   r0,       0 },
  { 0x09, 1, 1, inc,   r1,       0 },
  { 0x0A, 1, 1, inc,   r2,       0 },
  { 0x0B, 1, 1, inc,   r3,       0 },
  { 0x0C, 1, 1, inc,   r4,       0 },
  { 0x0D, 1, 1, inc,   r5,       0 },
  { 0x0E, 1, 1, inc,   r6,       0 },
  { 0x0F, 1, 1, inc,   r7,       0 },
  { 0x10, 3, 2, jbc,   bit_addr, comma,    rel_addr, 0 },
  { 0x11, 2, 2, acall, addr_11,  0 },
  { 0x12, 3, 2, lcall, addr_16,  0 },
  { 0x13, 1, 1, rrc,   a,        0 },
  { 0x14, 1, 1, dec,   a,        0 },
  { 0x15, 2, 1, dec,   addr_8,   0 },
  { 0x16, 1, 1, dec,   at_r0,    0 },
  { 0x17, 1, 1, dec,   at_r1,    0 },
  { 0x18, 1, 1, dec,   r0,       0 },
  { 0x19, 1, 1, dec,   r1,       0 },
  { 0x1A, 1, 1, dec,   r2,       0 },
  { 0x1B, 1, 1, dec,   r3,       0 },
  { 0x1C, 1, 1, dec,   r4,       0 },
  { 0x1D, 1, 1, dec,   r5,       0 },
  { 0x1E, 1, 1, dec,   r6,       0 },
  { 0x1F, 1, 1, dec,   r7,       0 },
  { 0x20, 3, 2, jb,    bit_addr, comma,    rel_addr, 0 },
  { 0x21, 2, 2, aj_dup,addr_11,  0 },
  { 0x22, 1, 2, ret,   0 },
  { 0x23, 1, 1, rl,    a,        0 },
  { 0x24, 2, 1, add,   a,        comma,    pound,    data,     0 },
  { 0x25, 2, 1, add,   a,        comma,    addr_8,   0 },
  { 0x26, 1, 1, add,   a,        comma,    at_r0,    0 },
  { 0x27, 1, 1, add,   a,        comma,    at_r1,    0 },
  { 0x28, 1, 1, add,   a,        comma,    r0,       0 },
  { 0x29, 1, 1, add,   a,        comma,    r1,       0 },
  { 0x2A, 1, 1, add,   a,        comma,    r2,       0 },
  { 0x2B, 1, 1, add,   a,        comma,    r3,       0 },
  { 0x2C, 1, 1, add,   a,        comma,    r4,       0 },
  { 0x2D, 1, 1, add,   a,        comma,    r5,       0 },
  { 0x2E, 1, 1, add,   a,        comma,    r6,       0 },
  { 0x2F, 1, 1, add,   a,        comma,    r7,       0 },
  { 0x30, 3, 2, jnb,   bit_addr, comma,    rel_addr, 0 },
  { 0x31, 2, 2, ac_dup,addr_11,  0 },
  { 0x32, 1, 2, reti,  0 },
  { 0x33, 1, 1, rlc,   a,        0 },
  { 0x34, 2, 1, addc,  a,        comma,    pound,    data,     0 },
  { 0x35, 2, 1, addc,  a,        comma,    addr_8,   0 },
  { 0x36, 1, 1, addc,  a,        comma,    at_r0,    0 },
  { 0x37, 1, 1, addc,  a,        comma,    at_r1,    0 },
  { 0x38, 1, 1, addc,  a,        comma,    r0,       0 },
  { 0x39, 1, 1, addc,  a,        comma,    r1,       0 },
  { 0x3A, 1, 1, addc,  a,        comma,    r2,       0 },
  { 0x3B, 1, 1, addc,  a,        comma,    r3,       0 },
  { 0x3C, 1, 1, addc,  a,        comma,    r4,       0 },
  { 0x3D, 1, 1, addc,  a,        comma,    r5,       0 },
  { 0x3E, 1, 1, addc,  a,        comma,    r6,       0 },
  { 0x3F, 1, 1, addc,  a,        comma,    r7,       0 },
  { 0x40, 2, 2, jc,    rel_addr, 0 },
  { 0x41, 2, 2, aj_dup,addr_11,  0 },
  { 0x42, 2, 1, orl,   addr_8,   comma,    a,        0 },
  { 0x43, 3, 2, orl,   addr_8,   comma,    pound,    data,     0 },
  { 0x44, 2, 1, orl,   a,        comma,    pound,    data,     0 },
  { 0x45, 2, 1, orl,   a,        comma,    addr_8,   0 },
  { 0x46, 1, 1, orl,   a,        comma,    at_r0,    0 },
  { 0x47, 1, 1, orl,   a,        comma,    at_r1,    0 },
  { 0x48, 1, 1, orl,   a,        comma,    r0,       0 },
  { 0x49, 1, 1, orl,   a,        comma,    r1,       0 },
  { 0x4A, 1, 1, orl,   a,        comma,    r2,       0 },
  { 0x4B, 1, 1, orl,   a,        comma,    r3,       0 },
  { 0x4C, 1, 1, orl,   a,        comma,    r4,       0 },
  { 0x4D, 1, 1, orl,   a,        comma,    r5,       0 },
  { 0x4E, 1, 1, orl,   a,        comma,    r6,       0 },
  { 0x4F, 1, 1, orl,   a,        comma,    r7,       0 },
  { 0x50, 2, 2, jnc,   rel_addr, 0 },
  { 0x51, 2, 2, ac_dup,addr_11,  0 },
  { 0x52, 2, 1, anl,   addr_8,   comma,    a,        0 },
  { 0x53, 3, 2, anl,   addr_8,   comma,    pound,    data,     0 },
  { 0x54, 2, 1, anl,   a,        comma,    pound,    data,     0 },
  { 0x55, 2, 1, anl,   a,        comma,    addr_8,   0 },
  { 0x56, 1, 1, anl,   a,        comma,    at_r0,    0 },
  { 0x57, 1, 1, anl,   a,        comma,    at_r1,    0 },
  { 0x58, 1, 1, anl,   a,        comma,    r0,       0 },
  { 0x59, 1, 1, anl,   a,        comma,    r1,       0 },
  { 0x5A, 1, 1, anl,   a,        comma,    r2,       0 },
  { 0x5B, 1, 1, anl,   a,        comma,    r3,       0 },
  { 0x5C, 1, 1, anl,   a,        comma,    r4,       0 },
  { 0x5D, 1, 1, anl,   a,        comma,    r5,       0 },
  { 0x5E, 1, 1, anl,   a,        comma,    r6,       0 },
  { 0x5F, 1, 1, anl,   a,        comma,    r7,       0 },
  { 0x60, 2, 2, jz,    rel_addr, 0 },
  { 0x61, 2, 2, aj_dup,addr_11,  0 },
  { 0x62, 2, 1, xrl,   addr_8,   comma,    a,        0 },
  { 0x63, 3, 2, xrl,   addr_8,   comma,    pound,    data,     0 },
  { 0x64, 2, 1, xrl,   a,        comma,    pound,    data,     0 },
  { 0x65, 2, 1, xrl,   a,        comma,    addr_8,   0 },
  { 0x66, 1, 1, xrl,   a,        comma,    at_r0,    0 },
  { 0x67, 1, 1, xrl,   a,        comma,    at_r1,    0 },
  { 0x68, 1, 1, xrl,   a,        comma,    r0,       0 },
  { 0x69, 1, 1, xrl,   a,        comma,    r1,       0 },
  { 0x6A, 1, 1, xrl,   a,        comma,    r2,       0 },
  { 0x6B, 1, 1, xrl,   a,        comma,    r3,       0 },
  { 0x6C, 1, 1, xrl,   a,        comma,    r4,       0 },
  { 0x6D, 1, 1, xrl,   a,        comma,    r5,       0 },
  { 0x6E, 1, 1, xrl,   a,        comma,    r6,       0 },
  { 0x6F, 1, 1, xrl,   a,        comma,    r7,       0 },
  { 0x70, 2, 2, jnz,   rel_addr, 0 },
  { 0x71, 2, 2, ac_dup,addr_11,  0 },
  { 0x72, 2, 2, orl,   c,        comma,    bit_addr, 0 },
  { 0x73, 1, 2, jmp,   a_dptr,   0 },
  { 0x74, 2, 1, mov,   a,        comma,    pound,    data,     0 },
  { 0x75, 3, 2, mov,   addr_8,   comma,    pound,    data,     0 },
  { 0x76, 2, 1, mov,   at_r0,    comma,    pound,    data,     0 },
  { 0x77, 2, 1, mov,   at_r1,    comma,    pound,    data,     0 },
  { 0x78, 2, 1, mov,   r0,       comma,    pound,    data,     0 },
  { 0x79, 2, 1, mov,   r1,       comma,    pound,    data,     0 },
  { 0x7A, 2, 1, mov,   r2,       comma,    pound,    data,     0 },
  { 0x7B, 2, 1, mov,   r3,       comma,    pound,    data,     0 },
  { 0x7C, 2, 1, mov,   r4,       comma,    pound,    data,     0 },
  { 0x7D, 2, 1, mov,   r5,       comma,    pound,    data,     0 },
  { 0x7E, 2, 1, mov,   r6,       comma,    pound,    data,     0 },
  { 0x7F, 2, 1, mov,   r7,       comma,    pound,    data,     0 },
  { 0x80, 2, 2, sjmp,  rel_addr, 0 },
  { 0x81, 2, 2, aj_dup,addr_11,  0 },
  { 0x82, 2, 2, anl,   c,        comma,    bit_addr, 0 },
  { 0x83, 1, 2, movc,  a,        comma,    a_pc,     0 },
  { 0x84, 1, 4, divab, ab,       0 },
  { 0x85, 3, 2, mov,   addr_8,   comma,    addr_8,   0 },
  { 0x86, 2, 2, mov,   addr_8,   comma,    at_r0,    0 },
  { 0x87, 2, 2, mov,   addr_8,   comma,    at_r1,    0 },
  { 0x88, 2, 2, mov,   addr_8,   comma,    r0,       0 },
  { 0x89, 2, 2, mov,   addr_8,   comma,    r1,       0 },
  { 0x8A, 2, 2, mov,   addr_8,   comma,    r2,       0 },
  { 0x8B, 2, 2, mov,   addr_8,   comma,    r3,       0 },
  { 0x8C, 2, 2, mov,   addr_8,   comma,    r4,       0 },
  { 0x8D, 2, 2, mov,   addr_8,   comma,    r5,       0 },
  { 0x8E, 2, 2, mov,   addr_8,   comma,    r6,       0 },
  { 0x8F, 2, 2, mov,   addr_8,   comma,    r7,       0 },
  { 0x90, 3, 2, mov,   dptr,     comma,    pound,    addr_16, 0 },
  { 0x91, 2, 2, ac_dup,addr_11,  0 },
  { 0x92, 2, 2, mov,   bit_addr, comma,    c,        0 },
  { 0x93, 1, 2, movc,  a,        comma,    a_dptr,   0 },
  { 0x94, 2, 1, subb,  a,        comma,    pound,    data,     0 },
  { 0x95, 2, 1, subb,  a,        comma,    addr_8,   0 },
  { 0x96, 1, 1, subb,  a,        comma,    at_r0,    0 },
  { 0x97, 1, 1, subb,  a,        comma,    at_r1,    0 },
  { 0x98, 1, 1, subb,  a,        comma,    r0,       0 },
  { 0x99, 1, 1, subb,  a,        comma,    r1,       0 },
  { 0x9A, 1, 1, subb,  a,        comma,    r2,       0 },
  { 0x9B, 1, 1, subb,  a,        comma,    r3,       0 },
  { 0x9C, 1, 1, subb,  a,        comma,    r4,       0 },
  { 0x9D, 1, 1, subb,  a,        comma,    r5,       0 },
  { 0x9E, 1, 1, subb,  a,        comma,    r6,       0 },
  { 0x9F, 1, 1, subb,  a,        comma,    r7,       0 },
  { 0xA0, 2, 2, orl,   c,        comma,    slash,    bit_addr,  0 },
  { 0xA1, 2, 2, aj_dup,addr_11,  0 },
  { 0xA2, 2, 1, mov,   c,        comma,    bit_addr, 0 },
  { 0xA3, 1, 2, inc,   dptr,     0 },
  { 0xA4, 1, 4, mul,   ab,       0 },
  { 0xA5, 1, 1, rsrvd, 0 },
  { 0xA6, 2, 2, mov,   at_r0,    comma,    addr_8,   0 },
  { 0xA7, 2, 2, mov,   at_r1,    comma,    addr_8,   0 },
  { 0xA8, 2, 2, mov,   r0,       comma,    addr_8,   0 },
  { 0xA9, 2, 2, mov,   r1,       comma,    addr_8,   0 },
  { 0xAA, 2, 2, mov,   r2,       comma,    addr_8,   0 },
  { 0xAB, 2, 2, mov,   r3,       comma,    addr_8,   0 },
  { 0xAC, 2, 2, mov,   r4,       comma,    addr_8,   0 },
  { 0xAD, 2, 2, mov,   r5,       comma,    addr_8,   0 },
  { 0xAE, 2, 2, mov,   r6,       comma,    addr_8,   0 },
  { 0xAF, 2, 2, mov,   r7,       comma,    addr_8,   0 },
  { 0xB0, 2, 2, anl,   c,        comma,    slash,    bit_addr,  0 },
  { 0xB1, 2, 2, ac_dup,addr_11,  0 },
  { 0xB2, 2, 1, cpl,   bit_addr, 0 },
  { 0xB3, 1, 1, cpl,   c,        0 },
  { 0xB4, 3, 2, cjne,  a,        comma,    pound,    data,     comma,    rel_addr, 0 },
  { 0xB5, 3, 2, cjne,  a,        comma,    addr_8,   comma,    rel_addr, 0 },
  { 0xB6, 3, 2, cjne,  at_r0,    comma,    pound,    data,     comma,    rel_addr, 0 },
  { 0xB7, 3, 2, cjne,  at_r1,    comma,    pound,    data,     comma,    rel_addr, 0 },
  { 0xB8, 3, 2, cjne,  r0,       comma,    pound,    data,     comma,    rel_addr, 0 },
  { 0xB9, 3, 2, cjne,  r1,       comma,    pound,    data,     comma,    rel_addr, 0 },
  { 0xBA, 3, 2, cjne,  r2,       comma,    pound,    data,     comma,    rel_addr, 0 },
  { 0xBB, 3, 2, cjne,  r3,       comma,    pound,    data,     comma,    rel_addr, 0 },
  { 0xBC, 3, 2, cjne,  r4,       comma,    pound,    data,     comma,    rel_addr, 0 },
  { 0xBD, 3, 2, cjne,  r5,       comma,    pound,    data,     comma,    rel_addr, 0 },
  { 0xBE, 3, 2, cjne,  r6,       comma,    pound,    data,     comma,    rel_addr, 0 },
  { 0xBF, 3, 2, cjne,  r7,       comma,    pound,    data,     comma,    rel_addr, 0 },
  { 0xC0, 2, 2, push,  addr_8,   0 },
  { 0xC1, 2, 2, aj_dup,addr_11,  0 },
  { 0xC2, 2, 1, clr,   bit_addr, 0 },
  { 0xC3, 1, 1, clr,   c,        0 },
  { 0xC4, 1, 1, swap,  a,        0 },
  { 0xC5, 2, 1, xch,   a,        comma,    addr_8,   0 },
  { 0xC6, 1, 1, xch,   a,        comma,    at_r0,    0 },
  { 0xC7, 1, 1, xch,   a,        comma,    at_r1,    0 },
  { 0xC8, 1, 1, xch,   a,        comma,    r0,       0 },
  { 0xC9, 1, 1, xch,   a,        comma,    r1,       0 },
  { 0xCA, 1, 1, xch,   a,        comma,    r2,       0 },
  { 0xCB, 1, 1, xch,   a,        comma,    r3,       0 },
  { 0xCC, 1, 1, xch,   a,        comma,    r4,       0 },
  { 0xCD, 1, 1, xch,   a,        comma,    r5,       0 },
  { 0xCE, 1, 1, xch,   a,        comma,    r6,       0 },
  { 0xCF, 1, 1, xch,   a,        comma,    r7,       0 },
  { 0xD0, 2, 2, pop,   addr_8,   0 },
  { 0xD1, 2, 2, ac_dup,addr_11,  0 },
  { 0xD2, 2, 1, setb,  bit_addr, 0 },
  { 0xD3, 1, 1, setb,  c,        0 },
  { 0xD4, 1, 1, da,    a,        0 },
  { 0xD5, 3, 2, djnz,  addr_8,   comma,    rel_addr, 0 },
  { 0xD6, 1, 1, xchd,  a,        comma,    at_r0,    0 },
  { 0xD7, 1, 1, xchd,  a,        comma,    at_r1,    0 },
  { 0xD8, 2, 2, djnz,  r0,       comma,    rel_addr, 0 },
  { 0xD9, 2, 2, djnz,  r1,       comma,    rel_addr, 0 },
  { 0xDA, 2, 2, djnz,  r2,       comma,    rel_addr, 0 },
  { 0xDB, 2, 2, djnz,  r3,       comma,    rel_addr, 0 },
  { 0xDC, 2, 2, djnz,  r4,       comma,    rel_addr, 0 },
  { 0xDD, 2, 2, djnz,  r5,       comma,    rel_addr, 0 },
  { 0xDE, 2, 2, djnz,  r6,       comma,    rel_addr, 0 },
  { 0xDF, 2, 2, djnz,  r7,       comma,    rel_addr, 0 },
  { 0xE0, 1, 2, movx,  a,        comma,    at_dptr,  0 },
  { 0xE1, 2, 2, aj_dup,addr_11,  0 },
  { 0xE2, 1, 2, movx,  a,        comma,    at_r0,    0 },
  { 0xE3, 1, 2, movx,  a,        comma,    at_r1,    0 },
  { 0xE4, 1, 1, clr,   a,        0 },
  { 0xE5, 2, 1, mov,   a,        comma,    addr_8,   0 },
  { 0xE6, 1, 1, mov,   a,        comma,    at_r0,    0 },
  { 0xE7, 1, 1, mov,   a,        comma,    at_r1,    0 },
  { 0xE8, 1, 1, mov,   a,        comma,    r0,       0 },
  { 0xE9, 1, 1, mov,   a,        comma,    r1,       0 },
  { 0xEA, 1, 1, mov,   a,        comma,    r2,       0 },
  { 0xEB, 1, 1, mov,   a,        comma,    r3,       0 },
  { 0xEC, 1, 1, mov,   a,        comma,    r4,       0 },
  { 0xED, 1, 1, mov,   a,        comma,    r5,       0 },
  { 0xEE, 1, 1, mov,   a,        comma,    r6,       0 },
  { 0xEF, 1, 1, mov,   a,        comma,    r7,       0 },
  { 0xF0, 1, 2, movx,  at_dptr,  comma,    a,        0 },
  { 0xF1, 2, 2, ac_dup,addr_11,  0 },
  { 0xF2, 1, 2, movx,  at_r0,    comma,    a,        0 },
  { 0xF3, 1, 2, movx,  at_r1,    comma,    a,        0 },
  { 0xF4, 1, 1, cpl,   a,        0 },
  { 0xF5, 2, 1, mov,   a,        comma,    addr_8,   0 },
  { 0xF6, 1, 1, mov,   at_r0,    comma,    a,        0 },
  { 0xF7, 1, 1, mov,   at_r1,    comma,    a,        0 },
  { 0xF8, 1, 1, mov,   r0,       comma,    a,        0 },
  { 0xF9, 1, 1, mov,   r1,       comma,    a,        0 },
  { 0xFA, 1, 1, mov,   r2,       comma,    a,        0 },
  { 0xFB, 1, 1, mov,   r3,       comma,    a,        0 },
  { 0xFC, 1, 1, mov,   r4,       comma,    a,        0 },
  { 0xFD, 1, 1, mov,   r5,       comma,    a,        0 },
  { 0xFE, 1, 1, mov,   r6,       comma,    a,        0 },
  { 0xFF, 1, 1, mov,   r7,       comma,    a,        0 },
  { 0x10, 3, 2, jbc,   bit_byte, dot,      bit_no,   comma,    rel_addr, 0 },
  { 0x20, 3, 2, jb,    bit_byte, dot,      bit_no,   comma,    rel_addr, 0 },
  { 0x30, 3, 2, jnb,   bit_byte, dot,      bit_no,   comma,    rel_addr, 0 },
  { 0x72, 2, 2, orl,   c,        comma,    bit_byte, dot,      bit_no,   0 },
  { 0x82, 2, 2, anl,   c,        comma,    bit_byte, dot,      bit_no,   0 },
  { 0x92, 2, 2, mov,   bit_byte, dot,      bit_no,   comma,    c,        0 },
  { 0xA0, 2, 2, orl,   c,        comma,    slash,    bit_byte, dot,      bit_no,   0 },
  { 0xA2, 2, 1, mov,   c,        comma,    bit_byte, dot,      bit_no,   0 },
  { 0xB0, 2, 2, anl,   c,        comma,    slash,    bit_byte, dot,      bit_no,   0 },
  { 0xB2, 2, 1, cpl,   bit_byte, dot,      bit_no,   0 },
  { 0xC2, 2, 1, clr,   bit_byte, dot,      bit_no,   0 },
  { 0xD2, 2, 1, setb,  bit_byte, dot,      bit_no,   0 },
  { UNDEF }
};

/* bitRange macro return true if 8-bit address is bit addressable
 * This is used in range checking 8051 . operator
 */
#define bitRange(x) ((x<BYTE_MAX)?bitRange_table[x]:UNDEF)

static const int bitRange_table[BYTE_MAX] =
  {
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
    0x00,  0x08,  0x10,  0x18,  0x20,  0x28,  0x30,  0x38,  
    0x40,  0x48,  0x50,  0x58,  0x60,  0x68,  0x70,  0x78,
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF,
    0x80,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0x88,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0x90,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0x98,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xA0,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xA8,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xB0,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xB8,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xC0,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xC8,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xD0,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xD8,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xE0,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xE8,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xF0,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, 
    0xF8,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF
  };

/* The array labels contains a name, value pair for each defined labels. Address labels and
 * equ labels currently go into the same name space. Array is limited and not ordered (to be fixed)
 * Inital value of array are all the register names in the SFR area
 */

const label_type def_labels[] = {
  { "acc",    0xE0 },
  { "auxc",   0xD6 },
  { "b",      0xF0 }, 
  { "carry",  0xD7 },
  { "dph",    0x83 }, 
  { "dpl",    0x82 }, 
  { "ie",     0xA8 },
  { "flag0",  0xD5 },
  { "ip",     0xB8 },
  { "ov",     0xD2 },
  { "p0",     0x80 }, 
  { "p1",     0x90 }, 
  { "p2",     0xA0 },
  { "p3",     0xB0 }, 
  { "parity", 0xD0 },
  { "pcon",   0x87 },
  { "psw",    0xD0 }, 
  { "rcap2h", 0xCB }, 
  { "rcap2l", 0xCA },
  { "rs1",    0xD4 },
  { "rs2",    0xD5 },
  { "sbuf",   0x99 }, 
  { "scon",   0x98 }, 
  { "sp",     0x81 }, 
  { "t2con",  0xC8 }, 
  { "tcon",   0x88 }, 
  { "th0",    0x8c }, 
  { "th1",    0x8D }, 
  { "th2",    0xCD }, 
  { "tl0",    0x8A }, 
  { "tl1",    0x8B }, 
  { "tl2",    0xCC }, 
  { "tmod",   0x89 }, 
  { "usrflg", 0xD1 },
  { "", UNDEF }
};

/* errMsg are the strings for the error values in errNo defined below
 * Note that errNo starts at 513.
 */
enum errNo 
  {
    addr11_range = CPUERR+1, bit_range, bitno_range, no_bitbyte, bit_syntax, direct_range
  };

const str_storage procErrMsg[] = 
  {
    "11-bit address out of range",
    "Bit address out of range", 
    "Bit number out of range",
    "Non bit addressable byte",
    "Bad bit address syntax", 
    "Direct address out of range"
  };

int handleInstr(const int *theInstr, int *tkn)
{
  int t;
  int tkn_pos = 0;
  int handleFlag = FALSE;

  memory[pc] = theInstr[INSTR_TKN_OP];

  if (theInstr[INSTR_TKN_OP] == 0x85) /* change order of mov direct, direct */
    {
      if (tkn[5]>BYTE_MAX || tkn[5]<0) longjmp(err, direct_range);
      memory[++pc] = tkn[5];

      if (tkn[2]>BYTE_MAX || tkn[2]<0) longjmp(err, direct_range);
      memory[++pc] = tkn[2];
      ++pc;
      return TRUE;
    }

  for (t=INSTR_TKN_INSTR+1; theInstr[t]; ++t)
    {
      switch (theInstr[t])
	{
	case bit_addr:
	  tkn_pos += 2;
	  if (tkn[tkn_pos]>BYTE_MAX || tkn[tkn_pos+2]<0) longjmp(err, bit_range);
	  memory[++pc] = tkn[tkn_pos];
	  handleFlag = TRUE;
	  break;
	case bit_byte: 
	  tkn_pos += 2;
	  if (bitRange(tkn[tkn_pos]) == UNDEF) longjmp(err, no_bitbyte);
	  break;
	case dot:
	  ++tkn_pos;
	  if (tkn[tkn_pos-2] != number || tkn[tkn_pos+1] != number)
	    longjmp(err, bit_syntax);
	  memory[++pc] = tkn[tkn_pos-1] + tkn[tkn_pos+2];
	  handleFlag = TRUE;
	  break;
	case bit_no:
	  tkn_pos += 2;
	  if (tkn[tkn_pos]<0 || tkn[tkn_pos]>7) longjmp(err, bitno_range);
	  break;
	case addr_11:
	  tkn_pos += 2;
	  if (tkn[tkn_pos]>0x7FF) longjmp(err, addr11_range);
	  memory[pc] |= (tkn[tkn_pos] & 0x700)/8;
	  memory[++pc] = getLow(tkn[tkn_pos]);
	  handleFlag = TRUE;
	  break;
	default:
	  ++tkn_pos;
	  break;
	}
    }
  if (handleFlag)
    {
      ++pc;
      return TRUE;
    }
  else
    return FALSE;
}

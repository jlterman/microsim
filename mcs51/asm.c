/*************************************************************************************

    Copyright (c) 2003, 2004 by James L. Terman
    This file is part of the 8051 Assembler backend

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
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#define CPU_LOCAL

#include "asmdefs.h"
#include "err.h"
#include "front.h"
#include "cpu.h"
#include "proc.h"
#include "version_cpu.h"

const char cpu_version[] = "8051 backend " CPU_VERS;

/* The array labels contains a name, value pair for each defined labels. Address labels and
 * equ labels currently go into the same name space. Array is limited and not ordered
 * Inital value of array are all the register names in the SFR area
 */
const label_type def_labels[] = {
  { "reset",  0x0000 },
  { "irq0",   0x0003 },
  { "timer0", 0x000B },
  { "irq1",   0x0013 },
  { "timer1", 0x001B },
  { "serial", 0x0023 },
  { "timer2", 0x002B },
  { "p0",     0x80 }, 
  { "sp",     0x81 }, 
  { "dpl",    0x82 }, 
  { "dph",    0x83 }, 
  { "pcon",   0x87 },
  { "tcon",   0x88 }, 
  { "tmod",   0x89 }, 
  { "tl0",    0x8A }, 
  { "tl1",    0x8B }, 
  { "th0",    0x8c }, 
  { "th1",    0x8D }, 
  { "p1",     0x90 }, 
  { "scon",   0x98 }, 
  { "sbuf",   0x99 }, 
  { "p2",     0xA0 },
  { "ie",     0xA8 },
  { "p3",     0xB0 }, 
  { "ip",     0xB8 },
  { "t2con",  0xC8 }, 
  { "rcap2l", 0xCA },
  { "rcap2h", 0xCB }, 
  { "tl2",    0xCC }, 
  { "th2",    0xCD }, 
  { "parity", 0xD0 },
  { "psw",    0xD0 }, 
  { "usrflg", 0xD1 },
  { "ov",     0xD2 },
  { "rs1",    0xD4 },
  { "rs2",    0xD5 },
  { "flag0",  0xD5 },
  { "auxc",   0xD6 },
  { "carry",  0xD7 },
  { "acc",    0xE0 },
  { "b",      0xF0 }, 
  { "", UNDEF }
};

/* The token array contains all the reserved words for 8051 assembly. 
 */
const int tokens_length = LAST_PROC_TOKEN - PROC_TOKEN;
const str_storage tokens[LAST_PROC_TOKEN - PROC_TOKEN] = 
  { 
    "#", ".",  "/", "@a+dptr", "@a+pc", "@dptr", "@r0", "@r1",
    "a", "ab", "acall", "add", "addc", "ajmp", "anl", "c", 
    "cjne", "clr", "cpl", "da", "dec", "div", "djnz", "dptr", 
    "inc", "jb", "jbc", "jc", "jmp", "jnb", "jnc", "jnz", 
    "jz", "lcall", "ljmp", "mov", "movc", "movx", "mul", "nop", 
    "orl", "pc", "pop", "push", "r0", "r1", "r2",  "r3", 
    "r4", "r5", "r6", "r7", "ret", "reti", "rl",  "rlc", 
    "rr", "rrc", "setb", "sjmp", "subb", "swap", "xch", "xchd", 
    "xrl"
  };

/* isRegister_table contains 0 if a token is not a register,
 *  otherwise it contains size of register in bytes
*/
const int isRegister_table[LAST_PROC_TOKEN - PROC_TOKEN] =
  {
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 2, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 
    0
  };

/* Macro returns true when character is a 1 char token
 * No intersection with isToken(x)
 * Any char listed here cannot be used in an expression in a parameter
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
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 0123456789:;<=>? */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* @ABCDEFGHIJKLMNO */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, /* PQRSTUVWXYZ[\]^_ */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* `abcdefghijklmno */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0  /* pqrstuvwxyz{|}~  */
  };

/* isInstr_table distinguishes tokens that represent 8051 instructions
 * and those that are tokens for the parameters of 8051 instructions.
 * includes psuedo-ops
 */
const int isInstr_table[LAST_PROC_TOKEN - PROC_TOKEN] = 
  { 
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 1, 1, 1, 1, 1, 0, 
    1, 1, 1, 1, 1, 1, 1, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1
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
  { 0x21, 2, 2, ajdup, addr_11,  0 },
  { 0x22, 1, 2, ret,   0 },
  { 0x23, 1, 1, rl,    a,        0 },
  { 0x24, 2, 1, add,   a,        comma,    pound,    data_8,   0 },
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
  { 0x31, 2, 2, acdup, addr_11,  0 },
  { 0x32, 1, 2, reti,  0 },
  { 0x33, 1, 1, rlc,   a,        0 },
  { 0x34, 2, 1, addc,  a,        comma,    pound,    data_8,   0 },
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
  { 0x41, 2, 2, ajdup, addr_11,  0 },
  { 0x42, 2, 1, orl,   addr_8,   comma,    a,        0 },
  { 0x43, 3, 2, orl,   addr_8,   comma,    pound,    data_8,   0 },
  { 0x44, 2, 1, orl,   a,        comma,    pound,    data_8,   0 },
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
  { 0x51, 2, 2, acdup, addr_11,  0 },
  { 0x52, 2, 1, anl,   addr_8,   comma,    a,        0 },
  { 0x53, 3, 2, anl,   addr_8,   comma,    pound,    data_8,   0 },
  { 0x54, 2, 1, anl,   a,        comma,    pound,    data_8,   0 },
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
  { 0x61, 2, 2, ajdup, addr_11,  0 },
  { 0x62, 2, 1, xrl,   addr_8,   comma,    a,        0 },
  { 0x63, 3, 2, xrl,   addr_8,   comma,    pound,    data_8,   0 },
  { 0x64, 2, 1, xrl,   a,        comma,    pound,    data_8,   0 },
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
  { 0x71, 2, 2, acdup, addr_11,  0 },
  { 0x72, 2, 2, orl,   c,        comma,    bit_addr, 0 },
  { 0x73, 1, 2, jmp,   a_dptr,   0 },
  { 0x74, 2, 1, mov,   a,        comma,    pound,    data_8,   0 },
  { 0x75, 3, 2, mov,   addr_8,   comma,    pound,    data_8,   0 },
  { 0x76, 2, 1, mov,   at_r0,    comma,    pound,    data_8,   0 },
  { 0x77, 2, 1, mov,   at_r1,    comma,    pound,    data_8,   0 },
  { 0x78, 2, 1, mov,   r0,       comma,    pound,    data_8,   0 },
  { 0x79, 2, 1, mov,   r1,       comma,    pound,    data_8,   0 },
  { 0x7A, 2, 1, mov,   r2,       comma,    pound,    data_8,   0 },
  { 0x7B, 2, 1, mov,   r3,       comma,    pound,    data_8,   0 },
  { 0x7C, 2, 1, mov,   r4,       comma,    pound,    data_8,   0 },
  { 0x7D, 2, 1, mov,   r5,       comma,    pound,    data_8,   0 },
  { 0x7E, 2, 1, mov,   r6,       comma,    pound,    data_8,   0 },
  { 0x7F, 2, 1, mov,   r7,       comma,    pound,    data_8,   0 },
  { 0x80, 2, 2, sjmp,  rel_addr, 0 },
  { 0x81, 2, 2, ajdup, addr_11,  0 },
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
  { 0x90, 3, 2, mov,   dptr,     comma,    pound,    data_16, 0 },
  { 0x91, 2, 2, acdup, addr_11,  0 },
  { 0x92, 2, 2, mov,   bit_addr, comma,    c,        0 },
  { 0x93, 1, 2, movc,  a,        comma,    a_dptr,   0 },
  { 0x94, 2, 1, subb,  a,        comma,    pound,    data_8,   0 },
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
  { 0xA0, 2, 2, orl,   c,        comma,    slash,        bit_addr,  0 },
  { 0xA1, 2, 2, ajdup, addr_11,  0 },
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
  { 0xB0, 2, 2, anl,   c,        comma,    slash,        bit_addr,  0 },
  { 0xB1, 2, 2, acdup, addr_11,  0 },
  { 0xB2, 2, 1, cpl,   bit_addr, 0 },
  { 0xB3, 1, 1, cpl,   c,        0 },
  { 0xB4, 3, 2, cjne,  a,        comma,    pound,    data_8,   comma,    rel_addr, 0 },
  { 0xB5, 3, 2, cjne,  a,        comma,    addr_8,   comma,    rel_addr, 0 },
  { 0xB6, 3, 2, cjne,  at_r0,    comma,    pound,    data_8,   comma,    rel_addr, 0 },
  { 0xB7, 3, 2, cjne,  at_r1,    comma,    pound,    data_8,   comma,    rel_addr, 0 },
  { 0xB8, 3, 2, cjne,  r0,       comma,    pound,    data_8,   comma,    rel_addr, 0 },
  { 0xB9, 3, 2, cjne,  r1,       comma,    pound,    data_8,   comma,    rel_addr, 0 },
  { 0xBA, 3, 2, cjne,  r2,       comma,    pound,    data_8,   comma,    rel_addr, 0 },
  { 0xBB, 3, 2, cjne,  r3,       comma,    pound,    data_8,   comma,    rel_addr, 0 },
  { 0xBC, 3, 2, cjne,  r4,       comma,    pound,    data_8,   comma,    rel_addr, 0 },
  { 0xBD, 3, 2, cjne,  r5,       comma,    pound,    data_8,   comma,    rel_addr, 0 },
  { 0xBE, 3, 2, cjne,  r6,       comma,    pound,    data_8,   comma,    rel_addr, 0 },
  { 0xBF, 3, 2, cjne,  r7,       comma,    pound,    data_8,   comma,    rel_addr, 0 },
  { 0xC0, 2, 2, push,  addr_8,   0 },
  { 0xC1, 2, 2, ajdup, addr_11,  0 },
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
  { 0xD1, 2, 2, acdup, addr_11,  0 },
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
  { 0xE1, 2, 2, ajdup, addr_11,  0 },
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
  { 0xF1, 2, 2, acdup, addr_11,  0 },
  { 0xF2, 1, 2, movx,  at_r0,    comma,    a,        0 },
  { 0xF3, 1, 2, movx,  at_r1,    comma,    a,        0 },
  { 0xF4, 1, 1, cpl,   a,        0 },
  { 0xF5, 2, 1, mov,   addr_8,   comma,    a,        0 },
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

  /* the follwing items are duplicate instruction but with alternate
   * parameters for assembler where baddr = bit_byte dot bit_no
   */
  { 0x10, 3, 2, jbc,   bit_byte, dot,      bit_no,   comma,    rel_addr, 0 },
  { 0x20, 3, 2, jb,    bit_byte, dot,      bit_no,   comma,    rel_addr, 0 },
  { 0x30, 3, 2, jnb,   bit_byte, dot,      bit_no,   comma,    rel_addr, 0 },
  { 0x72, 2, 2, orl,   c,        comma,    bit_byte, dot,      bit_no,   0 },
  { 0x82, 2, 2, anl,   c,        comma,    bit_byte, dot,      bit_no,   0 },
  { 0x92, 2, 2, mov,   bit_byte, dot,      bit_no,   comma,    c,        0 },
  { 0xA0, 2, 2, orl,   c,        comma,    slash,        bit_byte, dot,      bit_no,   0 },
  { 0xA2, 2, 1, mov,   c,        comma,    bit_byte, dot,      bit_no,   0 },
  { 0xB0, 2, 2, anl,   c,        comma,    slash,        bit_byte, dot,      bit_no,   0 },
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
    0xF8,  UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF, UNDEF
  };

/* errMsg are the strings for the error values in errNo defined below
 */
enum cpuErrNo 
  {
    no_bitbyte = LAST_ASM_ERR,
    bit_syntax, LAST_CPU_ERRNO
  };

const str_storage cpuErrMsg[LAST_CPU_ERRNO - LAST_ASM_ERR] = 
  {
    "Non bit addressable byte",
    "Bad bit address syntax"
  };

/* The function isInstr distinguishes tokens that represent 8051 instructions
 * and those that are tokens for the parameters of 8051 instructions.
 * includes psuedo-ops
 */
int isInstrOp(int token)
{
  if (token<LAST_PROC_TOKEN && token>=PROC_TOKEN)
    return isInstr_table[token - PROC_TOKEN];
  else
    return 0;
}

/* handleInstr handles 8051 instruction mov addr_8, addr_8 is 
 * mov src, dst not mov dst, src 
 */
int handleInstr(const int *theInstr, int *tkn)
{
  memory[pc] = theInstr[INSTR_TKN_OP];

  if (theInstr[INSTR_TKN_OP] == 0x85) /* change order of mov addr_8, addr_8 */
    {
      memory[++pc] = tkn[5];
      memory[++pc] = tkn[2];
      ++pc;
      return TRUE;
    }
  return FALSE;
}

/* handleTkn() handles tokens that are 8051 specific tokens
 * bit_addr, bit_byte, dot, bit_no and addr_11.
 */
int handleTkn(const int theInstrTkn, int *tkn, int *tkn_pos)
{
  int n, addr, handleFlag = TRUE;

  switch (theInstrTkn)
    {
    case bit_byte: /* bit_byte is a bit addressable byte. Check this */
      *tkn_pos += 2;
      if (bitRange(tkn[*tkn_pos]) == UNDEF) longjmp(err, no_bitbyte);
      memory[pc] = tkn[*tkn_pos];
      break;
    case dot:  /* dot tokens means add bit byte to bit no afterwards */
      ++*tkn_pos;
      break; 
    case bit_no:
      *tkn_pos += 2;
      if (tkn[*tkn_pos] > 7 || tkn[*tkn_pos] < 0) 
	longjmp(err, addr3_range);
      memory[pc++] += tkn[*tkn_pos];
      break;
    case addr_11: 
      *tkn_pos += 2;
      if (tkn[*tkn_pos] > ((1<<11) - 1) || tkn[*tkn_pos] < 0) 
	longjmp(err, addr1_range + n);

      /* Generate 11 bit addr from upper 5 bits of pc and 11 bits of 
       * target address given by assembly line.
       */
      addr = (pc & 0xF800) + (tkn[*tkn_pos] & 0x7FF);
      memory[pc-1] |= (tkn[*tkn_pos] & 0x700)/8;
      memory[pc++] = getLow(tkn[*tkn_pos]);
      break;
    default:
      handleFlag = FALSE;
      break;
    }

  return handleFlag;
}

/*  isConstProcTkn returns true if token is cpu specific token is constant
 */
int isConstProcTkn(int t)
{
  return (t>CONST_PROC_TOKEN);
}

/* isJSR will return TRUE is opcode is a subroutine call instruction
 */
int isJSR(int opcode)
{
  int t = cpu_instr_tkn[opcode][INSTR_TKN_INSTR];
  return (t == acall || t == acdup || t == lcall);
}

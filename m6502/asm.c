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

const char cpu_version[] = "6502 backend " CPU_VERS;

/* The token array contains all the reserved words for 6502 assembly. 
 */
const int tokens_length = LAST_PROC_TOKEN - PROC_TOKEN;
const str_storage tokens[LAST_PROC_TOKEN - PROC_TOKEN] = 
  { 
    "#"  ,   "(",   ")",   "a", "adc", "add", "and", "asl",
    "bcc", "bcs", "beq", "bit", "bmi", "bne", "bpl", "brk",
    "bvc", "bvs", "clc", "cld", "cli", "clv", "cmp", "cpx",
    "cpy", "dec", "dex", "dey", "eor", "inc", "inx", "iny",
    "jmp", "jsr", "lda", "ldx", "ldy", "lsr", "nop", "NOP",
    "ora",   "p",  "pc", "pha", "php", "pla", "plp", "rol",
    "ror", "rti", "rts", "sbc", "sec", "sed", "sei",  "sp",
    "sta", "stx", "sty", "tax", "tay", "tsx", "txa", "txs", 
    "tya",   "x",   "y"
  };

/* isRegister_table contains 0 if a token is not a register,
 *  otherwise it contains size of register in bytes
*/
const int isRegister_table[LAST_PROC_TOKEN - PROC_TOKEN] =
  {
    0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1
  };

/* Macro returns true when character is a 1 char token
 * Note: no intersection with isToken(x)
 * Any char listed here cannot be used in an expression in a parameter
 */
const int isCharToken_table[ASCII_MAX] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, /*  !"#$%&'()*+,-./ */
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
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  !"#$%&'()*+,-./ */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 0123456789:;<=>? */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* @ABCDEFGHIJKLMNO */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, /* PQRSTUVWXYZ[\]^_ */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* `abcdefghijklmno */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0  /* pqrstuvwxyz{|}~  */
  };

/* isInstr_table distinguishes tokens that represent 6502 instructions
 * and those that are tokens for the parameters of 6502 instructions.
 * includes psuedo-ops
 */
const int isInstr_table[LAST_PROC_TOKEN - PROC_TOKEN] = 
  { 
    0, 0, 0, 0, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 0, 0, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 0, 
    1, 1, 1, 1, 1, 1, 1, 1, 
    1, 0, 0
};

/* This array contains all the parameter info 
 * for every instruction in the 6502.
 */
const int cpu_instr_tkn[][INSTR_TKN_BUF] = 
{
  { 0x00, 1, 0, brk,   0 },
  { 0x01, 2, 0, ora,   leftPar,  addr_8,   comma,    x,        rightPar, 0 },
  { 0x02, 1, 0, NOP,   0 },
  { 0x03, 1, 0, NOP,   0 },
  { 0x04, 1, 0, NOP,   0 },
  { 0x05, 2, 0, ora,   addr_8,   0 },
  { 0x06, 2, 0, asl,   addr_8 },
  { 0x07, 1, 0, NOP,   0 },
  { 0x08, 1, 0, php,   0 },
  { 0x09, 2, 0, ora,   pound,    data_8,   0 },
  { 0x0A, 1, 0, asl,   a,        0 },
  { 0x0B, 1, 0, NOP,   0 },
  { 0x0C, 1, 0, NOP,   0 },
  { 0x0D, 3, 0, ora,   addr_16,  0 },
  { 0x0E, 3, 0, asl,   addr_16,  0 },
  { 0x0F, 1, 0, NOP,   0 },
  { 0x10, 2, 0, bpl,   rel_addr, 0 },
  { 0x11, 2, 0, ora,   leftPar,   addr_8,  rightPar, comma,    y,        0 },
  { 0x12, 1, 0, NOP,   0 },
  { 0x13, 1, 0, NOP,   0 },
  { 0x14, 1, 0, NOP,   0 },
  { 0x15, 2, 0, ora,   addr_8,   comma,    x,        0 },
  { 0x16, 2, 0, asl,   addr_8,   comma,    x,        0 },
  { 0x17, 1, 0, NOP,   0 },
  { 0x18, 1, 0, clc,   0 },
  { 0x19, 3, 0, ora,   addr_16,  comma,    y,        0 },
  { 0x1A, 1, 0, NOP,   0 },
  { 0x1B, 1, 0, NOP,   0 },
  { 0x1C, 1, 0, NOP,   0 },
  { 0x1D, 3, 0, ora,   addr_16,  comma,    x,        0 },
  { 0x1E, 3, 0, asl,   addr_16,  comma,    x,        0 },
  { 0x1F, 1, 0, NOP,   0 },
  { 0x20, 3, 0, jsr,   addr_16,  0 },
  { 0x21, 2, 0, and,   leftPar,  addr_8,   comma,    x,        rightPar, 0 },
  { 0x22, 1, 0, NOP,   0 },
  { 0x23, 1, 0, NOP,   0 },
  { 0x24, 2, 0, bit,   addr_8,   0 },
  { 0x25, 2, 0, and,   addr_8,   0 },
  { 0x26, 2, 0, rol,   addr_8,   0 },
  { 0x27, 1, 0, NOP,   0 },
  { 0x28, 1, 0, plp,   0 },
  { 0x29, 2, 0, and,   pound,    data_8,   0 },
  { 0x2A, 1, 0, rol,   a,        0 },
  { 0x2B, 1, 0, NOP,   0 },
  { 0x2C, 3, 0, bit,   addr_16,  0 },
  { 0x2D, 3, 0, and,   addr_16,  0 },
  { 0x2E, 3, 0, rol,   addr_16,  0 },
  { 0x2F, 1, 0, NOP,   0 },
  { 0x30, 2, 0, bmi,   rel_addr, 0 },
  { 0x31, 2, 0, and,   leftPar,  addr_8,   rightPar, comma,   y,         0 },
  { 0x32, 1, 0, NOP,   0 },
  { 0x33, 1, 0, NOP,   0 },
  { 0x34, 1, 0, NOP,   0 },
  { 0x35, 2, 0, and,   leftPar,  addr_8,   rightPar, comma,   x,         0 },
  { 0x36, 2, 0, rol,   leftPar,  addr_8,   rightPar, comma,   x,         0 },
  { 0x37, 1, 0, NOP,   0 },
  { 0x38, 1, 0, sec,   0 },
  { 0x39, 3, 0, and,   addr_16,  comma,    y,        0 },
  { 0x3A, 1, 0, NOP,   0 },
  { 0x3B, 1, 0, NOP,   0 },
  { 0x3C, 1, 0, NOP,   0 },
  { 0x3D, 3, 0, and,   addr_16,  comma,    x,        0 },
  { 0x3E, 3, 0, rol,   addr_16,  comma,    y,        0 },
  { 0x3F, 1, 0, NOP,   0 },
  { 0x40, 1, 0, rti,   0 },
  { 0x41, 2, 0, eor,   leftPar,  addr_8,   comma,    x,       rightPar,  0 },
  { 0x42, 1, 0, NOP,   0 },
  { 0x43, 1, 0, NOP,   0 },
  { 0x44, 1, 0, NOP,   0 },
  { 0x45, 2, 0, eor,   addr_8,   0 },
  { 0x46, 2, 0, lsr,   addr_8,   0 },
  { 0x47, 1, 0, NOP,   0 },
  { 0x48, 1, 0, pha,   0 },
  { 0x49, 2, 0, eor,   pound,    data_8,   0 },
  { 0x4A, 1, 0, lsr,   a,        0 },
  { 0x4B, 1, 0, NOP,   0 },
  { 0x4C, 3, 0, jmp,   addr_16,  0 },
  { 0x4D, 3, 0, eor,   addr_16,  0 },
  { 0x4E, 3, 0, lsr,   addr_16,  0 },
  { 0x4F, 1, 0, NOP,   0 },
  { 0x50, 2, 0, bvc,   rel_addr, 0 },
  { 0x51, 2, 0, eor,   leftPar,  addr_8,   rightPar, comma,   y,         0 },
  { 0x52, 1, 0, NOP,   0 },
  { 0x53, 1, 0, NOP,   0 },
  { 0x54, 1, 0, NOP,   0 },
  { 0x55, 2, 0, eor,   addr_8,   comma,    x,        0 },
  { 0x56, 2, 0, lsr,   addr_8,   comma,    x,        0 },
  { 0x57, 1, 0, NOP,   0 },
  { 0x58, 1, 0, cli,   0 },
  { 0x59, 3, 0, eor,   addr_16,  comma,    y,        0 },
  { 0x5A, 1, 0, NOP,   0 },
  { 0x5B, 1, 0, NOP,   0 },
  { 0x5C, 1, 0, NOP,   0 },
  { 0x5D, 3, 0, eor,   addr_16,  comma,    x,        0 },
  { 0x5E, 3, 0, lsr,   addr_16,  comma,    x,        0 },
  { 0x5F, 1, 0, NOP,   0 },
  { 0x60, 1, 0, rts,   0 },
  { 0x61, 2, 0, adc,   leftPar,  addr_8,   comma,    x,       rightPar,  0 },
  { 0x62, 1, 0, NOP,   0 },
  { 0x63, 1, 0, NOP,   0 },
  { 0x64, 1, 0, NOP,   0 },
  { 0x65, 2, 0, adc,   addr_8,   0 },
  { 0x66, 2, 0, ror,   addr_8,   0 },
  { 0x67, 1, 0, NOP,   0 },
  { 0x68, 1, 0, pla,   0 },
  { 0x69, 2, 0, adc,   pound,    data_8,   0 },
  { 0x6A, 1, 0, ror,   a,        0 },
  { 0x6B, 1, 0, NOP,   0 },
  { 0x6C, 3, 0, jmp,   leftPar,  addr_16,  rightPar, 0 },
  { 0x6D, 3, 0, adc,   addr_16,  0 },
  { 0x6E, 3, 0, ror,   addr_16,  0 },
  { 0x6F, 1, 0, NOP,   0 },
  { 0x70, 2, 0, bvs,   rel_addr, 0 },
  { 0x71, 2, 0, adc,   leftPar,  addr_8,   rightPar, 0 },
  { 0x72, 1, 0, NOP,   0 },
  { 0x73, 1, 0, NOP,   0 },
  { 0x74, 1, 0, NOP,   0 },
  { 0x75, 2, 0, adc,   addr_8,   comma,    x,        0 },
  { 0x76, 2, 0, adc,   addr_8,   comma,    x,        0 },
  { 0x77, 1, 0, NOP,   0 },
  { 0x78, 1, 0, sei,   0 },
  { 0x79, 3, 0, adc,   addr_16,  comma,    y,        0 },
  { 0x7A, 1, 0, NOP,   0 },
  { 0x7B, 1, 0, NOP,   0 },
  { 0x7C, 1, 0, NOP,   0 },
  { 0x7D, 3, 0, adc,   addr_16,  comma,    x,        0 },
  { 0x7E, 3, 0, ror,   addr_16,  comma,    x,        0 },
  { 0x7F, 1, 0, NOP,   0 },
  { 0x80, 1, 0, NOP,   0 },
  { 0x81, 2, 0, sta,   leftPar,  addr_8,   comma,    x,       rightPar,  0 },
  { 0x82, 1, 0, NOP,   0 },
  { 0x83, 1, 0, NOP,   0 },
  { 0x84, 2, 0, sty,   addr_8,   0 },
  { 0x85, 2, 0, sta,   addr_8,   0 },
  { 0x86, 2, 0, stx,   addr_8,   0 },
  { 0x87, 1, 0, NOP,   0 },
  { 0x88, 1, 0, dey,   0 },
  { 0x89, 1, 0, NOP,   0 },
  { 0x8A, 1, 0, txa,   0 },
  { 0x8B, 1, 0, NOP,   0 },
  { 0x8C, 3, 0, sty,   addr_16,  0 },
  { 0x8D, 3, 0, sta,   addr_16,  0 },
  { 0x8E, 3, 0, stx,   addr_16,  0 },
  { 0x8F, 1, 0, NOP,   0 },
  { 0x90, 2, 0, bcc,   rel_addr, 0 },
  { 0x91, 2, 0, sta,   leftPar,  addr_8,   rightPar, comma,   y,         0 },
  { 0x92, 1, 0, NOP,   0 },
  { 0x93, 1, 0, NOP,   0 },
  { 0x94, 2, 0, sty,   addr_8,   comma,    x,        0 },
  { 0x95, 2, 0, sta,   addr_8,   comma,    x,        0 },
  { 0x96, 2, 0, stx,   addr_8,   comma,    y,        0 },
  { 0x97, 1, 0, NOP,   0 },
  { 0x98, 1, 0, tya,   0 },
  { 0x99, 3, 0, sta,   addr_16,  comma,    y,        0 },
  { 0x9A, 1, 0, txs,   0 },
  { 0x9B, 1, 0, NOP,   0 },
  { 0x9C, 1, 0, NOP,   0 },
  { 0x9D, 3, 0, sta,   addr_16,  comma,    x,        0 },
  { 0x9E, 1, 0, NOP,   0 },
  { 0x9F, 1, 0, NOP,   0 },
  { 0xA0, 2, 0, ldy,   pound,    data_8,   0 },
  { 0xA1, 2, 0, lda,   leftPar,  addr_8,   comma,    x,       rightPar,  0 },
  { 0xA2, 2, 0, ldx,   pound,    data_8,   0 },
  { 0xA3, 1, 0, NOP,   0 },
  { 0xA4, 2, 0, ldy,   pound,    addr_8,   0 },
  { 0xA5, 2, 0, lda,   addr_8,   0 },
  { 0xA6, 2, 0, ldx,   addr_8,   0 },
  { 0xA7, 1, 0, NOP,   0 },
  { 0xA8, 1, 0, tay,   0 },
  { 0xA9, 2, 0, lda,   pound,    data_8,   0 },
  { 0xAA, 1, 0, tax,   0 },
  { 0xAB, 1, 0, NOP,   0 },
  { 0xAC, 3, 0, ldy,   addr_16,  0 },
  { 0xAD, 3, 0, lda,   addr_16,  0 },
  { 0xAE, 3, 0, ldx,   addr_16,  0 },
  { 0xAF, 1, 0, NOP,   0 }, 
  { 0xB0, 2, 0, bcs,   rel_addr, 0 },
  { 0xB1, 2, 0, lda,   leftPar,  addr_8,   rightPar, comma,   y,         0 },
  { 0xB2, 1, 0, NOP,   0 },
  { 0xB3, 1, 0, NOP,   0 },
  { 0xB4, 2, 0, ldy,   addr_8,   comma,    x,        0 },
  { 0xB5, 2, 0, lda,   addr_8,   comma,    x,        0 },
  { 0xB6, 2, 0, ldx,   addr_8,   comma,    y,        0 },
  { 0xB7, 1, 0, NOP,   0 },
  { 0xB8, 1, 0, clv,   0 },
  { 0xB9, 3, 0, lda,   addr_16,  comma,    y,        0 },
  { 0xBA, 1, 0, tsx,   0 },
  { 0xBB, 1, 0, NOP,   0 },
  { 0xBC, 3, 0, ldy,   addr_16,  comma,    x,        0 },
  { 0xBD, 3, 0, lda,   addr_16,  comma,    x,        0 },
  { 0xBE, 3, 0, ldx,   addr_16,  comma,    y,        0 },
  { 0xBF, 1, 0, NOP,   0 },
  { 0xC0, 2, 0, cpy,   pound,    data_8,   0 },
  { 0xC1, 2, 0, cmp,   leftPar,  addr_8,   comma,    x,       rightPar,  0 },
  { 0xC2, 1, 0, NOP,   0 },
  { 0xC3, 1, 0, NOP,   0 },
  { 0xC4, 2, 0, cpy,   addr_8,   0 },
  { 0xC5, 2, 0, cmp,   addr_8,   0 },
  { 0xC6, 2, 0, dec,   addr_8,   0 },
  { 0xC7, 1, 0, NOP,   0 },
  { 0xC8, 1, 0, iny,   0 },
  { 0xC9, 2, 0, cmp,   pound,    data_8,   0 },
  { 0xCA, 1, 0, dex,   0 },
  { 0xCB, 1, 0, NOP,   0 },
  { 0xCC, 3, 0, cpy,   addr_16,  0 },
  { 0xCD, 3, 0, cmp,   addr_16,  0 },
  { 0xCE, 3, 0, dec,   addr_16,  0 },
  { 0xCF, 1, 0, NOP,   0 },
  { 0xD0, 2, 0, bne,   rel_addr, 0 },
  { 0xD1, 2, 0, cmp,   leftPar,  addr_8,   rightPar, comma,   y,         0 },
  { 0xD2, 1, 0, NOP,   0 },
  { 0xD3, 1, 0, NOP,   0 },
  { 0xD4, 1, 0, NOP,   0 },
  { 0xD5, 2, 0, cmp,   addr_8,   comma,    x,        0 },
  { 0xD6, 2, 0, dec,   addr_8,   comma,    x,        0 },
  { 0xD7, 1, 0, NOP,   0 },
  { 0xD8, 1, 0, cld,   0 },
  { 0xD9, 3, 0, cmp,   addr_16,  comma,    y,        0 },
  { 0xDA, 1, 0, NOP,   0 },
  { 0xDB, 1, 0, NOP,   0 },
  { 0xDC, 1, 0, NOP,   0 },
  { 0xDD, 3, 0, cmp,   addr_16,  comma,    x,        0 },
  { 0xDE, 3, 0, dec,   addr_16,  comma,    x,        0 },
  { 0xDF, 1, 0, NOP,   0 },
  { 0xE0, 2, 0, cpx,   pound,    data_8,   0 },
  { 0xE1, 2, 0, sbc,   leftPar,  addr_8,   comma,    x,       rightPar,  0 },
  { 0xE2, 1, 0, NOP,   0 },
  { 0xE3, 1, 0, NOP,   0 },
  { 0xE4, 2, 0, cpx,   addr_8,   0 },
  { 0xE5, 2, 0, sbc,   addr_8,   0 },
  { 0xE6, 2, 0, inc,   addr_8,   0 },
  { 0xE7, 1, 0, NOP,   0 },
  { 0xE8, 1, 0, inx,   0 },
  { 0xE9, 2, 0, sbc,   pound,    data_8,   0 },
  { 0xEA, 1, 0, nop,   0 },
  { 0xEB, 1, 0, NOP,   0 },
  { 0xEC, 3, 0, cpx,   addr_16,  0 },
  { 0xED, 3, 0, sbc,   addr_16,  0 },
  { 0xEE, 3, 0, inc,   addr_16,  0 },
  { 0xEF, 1, 0, NOP,   0 },
  { 0xF0, 2, 0, beq,   rel_addr, 0 },
  { 0xF1, 2, 0, sbc,   leftPar,  addr_8,   rightPar, comma,   y,         0 },
  { 0xF2, 1, 0, NOP,   0 },
  { 0xF3, 1, 0, NOP,   0 },
  { 0xF4, 1, 0, NOP,   0 },
  { 0xF5, 2, 0, sbc,   addr_8,   comma,    x,        0 },
  { 0xF6, 2, 0, inc,   addr_8,   comma,    x,        0 },
  { 0xF7, 1, 0, NOP,   0 },
  { 0xF8, 1, 0, sed,   0 },
  { 0xF9, 3, 0, sbc,   addr_16,  comma,    y,        0 },
  { 0xFA, 1, 0, NOP,   0 },
  { 0xFB, 1, 0, NOP,   0 },
  { 0xFC, 1, 0, NOP,   0 },
  { 0xFD, 3, 0, sbc,   addr_16,  comma,    x,        0 },
  { 0xFE, 3, 0, inc,   addr_16,  comma,    x,        0 },
  { 0xFF, 1, 0, NOP,   0 },
  { UNDEF }
};

/* Predefined labels needed for 6502
 */

const label_type def_labels[] = {
  { "nmi",   0xFFFA },
  { "reset", 0xFFFC },
  { "irq",   0xFFFE },
  { "", UNDEF }
};

/* no 6502 specific error messages
 */
const str_storage cpuErrMsg[] =  { "" };

/* The function isInstr distinguishes tokens that represent 6502 instructions
 * and those that are tokens for the parameters of 6502 instructions.
 * includes psuedo-ops
 */
int isInstrOp(int token)
{
  if (token<LAST_PROC_TOKEN && token>=PROC_TOKEN)
    return isInstr_table[token - PROC_TOKEN];
  else
    return 0;
}

/* No 6502 specific instruction handling
 */
int handleInstr(const int *theInstr, int *tkn)
{
  return FALSE;
}

/* No 6502 specific tokens
 */
int handleTkn(const int theInstrTkn, int *tkn, int *tkn_pos)
{
  return FALSE;
}

/* no 6502 specific constant tokens
 */
int isConstProcTkn(int t)
{
  return FALSE;
}

/* isJSR will return TRUE is opcode is a subroutine call instruction
 */
int isJSR(int opcode)
{
  int t = cpu_instr_tkn[opcode][INSTR_TKN_INSTR];
  return (t == jsr);
}

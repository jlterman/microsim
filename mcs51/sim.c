/*************************************************************************************

    Copyright (c) 2003, 2004 by James L. Terman
    This file is part of the 8051 Simulator backend

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
#include <string.h>
#include <assert.h>

#define SIM_CPU_LOCAL

#include "asmdefs.h"
#include "asm.h"
#include "front.h"
#include "proc.h"
#include "cpu.h"

/* atRam will return the value of the memory location accessible by
 * the 8051 @Ri addressing mode. The upper 128 bytes of @Ri address
 * space is between 0x100 and 0x180.
 */
#define atram(x) (ram[((x)>=BYTE_MAX/2) ? (x)+BYTE_MAX/2 : (x)])

/* for bit addrr x, bit has the bit number and addr the byte address
 */
#define bitAddr(x, addr, bit) (bit = 1<<(x % 8), addr = ram + ((x>SGN_BYTE_MAX) ? x & 0xF8 : 0x20 + x/8))

/* defintions for PSW */

#define carry 128
#define auxc   64
#define rs1    16
#define rs0     8
#define ov      4
#define prty    1

#define setPSW(x, m) setBit(x, ram + PSW, m);
#define setC(x) setBit(x, ram + PSW, carry)
#define getC() (ram[PSW] >= carry)

/* allocation of memory location and SFR's
 * ram is 384 bytes. The first 128 bytes are the joint indirect and direct
 * address space. The next 128 bytes are the SFR accessible by direct address.
 * The last 128 bytes are the upper portion of the indirect adress space
 * use the macro atram to get the correct area
 */
static int ram[BYTE_MAX+BYTE_MAX/2] = { 0 }; /* internal ram */
static int xram[MEMORY_MAX] = { 0 };         /* external RAM */
static int *reg[8];                          /* address of data registers */

/* add 1 to stack pointer and store value at @Ri address
 */
static void pushStack(int data)
{
  if (++(ram[SP]) == (BYTE_MAX-1)) ram[SP] = 0;
  atram(ram[SP]) = data;
}

/* return value pointed to by stack ponter in @Ri address and decrement
 */
static int popStack(void)
{
  int value = atram(ram[SP]); --(ram[SP]);
  return value;
}

/* Put cpu in correct state after reset
 */
void reset(void)
{
  int i;
  for (i = 0; i<8; ++i) reg[i] = ram + i;
  ram[P0] = ram[P1] = ram[P2] = ram[P3] = BYTE_MASK;
  pc = RESET;
  ram[SP] = 7;
}

int irq(int i)
{
  if (i<0 || i>1) return UNDEF;
  if (!(ram[IE] & (i*2))) return FALSE;
  pushStack(getLow(pc));
  pushStack(getHigh(pc));
  pc = (i) ? IRQ1 : IRQ0;
  return TRUE;
}

/* add value to acc with the proper setting of status registers.
 * carry is used according to carryFlag but always set afterwards
 */
static void doAdd(int value, int carryFlag)
{
  int ac = ((ram[ACC] & LO_NYBLE) + (value & LO_NYBLE) + getC()*carryFlag) > LO_NYBLE;
  int nc = (ram[ACC] + value + (getC()*carryFlag)) > BYTE_MASK;
  setPSW(ac, auxc);
  ac = ((ram[ACC] & (BYTE_MASK - BIT7_MASK)) + (value & (BYTE_MASK - BIT7_MASK)) + getC()*carryFlag) > (BYTE_MASK - BIT7_MASK);
  setPSW(ac ^ nc, ov);

  setC(nc);
  ram[ACC] = (ram[ACC] + value + getC()*carryFlag) & BYTE_MASK;
}

/* doSub sub value from acc setting status flags correctly afterwards
 */
static void doSub(int value)
{
  int ac = ((ram[ACC] & LO_NYBLE) - (value & LO_NYBLE) - getC()) < 0;
  int nc = (ram[ACC] - value - getC()) < 0;

  setPSW(ac, auxc);
  ac = ((ram[ACC] & (BYTE_MASK - BIT7_MASK)) - (value & (BYTE_MASK - BIT7_MASK)) - getC()) < 0;
  setPSW(ac ^ nc, ov); setC(nc);
  ram[ACC] = (ram[ACC] - value - getC()) & BYTE_MASK;
}

/* after each instr PSW can change. Address of data registers change if
 * rs0 and rs1 are changed. prty bit is on if odd no of bits in acc.
 */
static void updatePSW()
{
  int i, p, addr = ram[PSW] & (rs1 + rs0);
  if (reg[0] - ram != addr)
    {
      reg[0] = ram + addr;     reg[1] = ram + addr + 1; 
      reg[2] = ram + addr + 2; reg[3] = ram + addr + 3;
      reg[4] = ram + addr + 4; reg[5] = ram + addr + 5; 
      reg[6] = ram + addr + 6; reg[7] = ram + addr + 7;
    }
  p = 0;
  for (i=BIT7_MASK; i; i /= 2) { p += ((ram[ACC] & i) != 0); }
  setPSW(p & 1, prty);
}

/* Get param will calculate the parameters from the cpu_instr_tkn[op] entry
 * *index points to the first parameter of the opcode, and *code points
 * to the next byte in code memory after the opcode
 * 
 * if reg or memory parameter found, *param is set to address of memory or reg
 * if regular addr found, *addr set to this value
 * if bit addr found, *param will have addr of byte, and bparam has bit num
 */
static int getParam(int op, const int **index, int **code, int **param, int *bparam, int *addr)
{
  int inv = 1, *p = NULL, bp = UNDEF;

  switch (**index)
    {
    case 0:
      return FALSE;
      break;
    case addr_11: /* mask top 3 bits of opcode and make a10, a9, a8 */
      *addr = ((op & 0xE0)<<3) + **code;
      break;
    case addr_16:
      *addr = **code*BYTE_MAX +  *++*code;
      break;
    case addr_8: /* addr_8 is either or a src or a destination */
      p = ram + **code;
      break;
    case rel_addr:
      *addr = **code;
      break;      
    case pound: /* code in memory is the source of the move */
      p = *code;
      ++*index;
      break;
    case slash: /* negative bit addr will indicate inverse bit */
      ++*index;
      inv = -1;
    case bit_addr:
      bitAddr(**code, p, bp);
      bp *= inv;
      break;
    case a_dptr:
      *addr = ram[DPL] + ram[DPH]*BYTE_MAX + ram[ACC];
      break;
    case a_pc:
      *addr = pc + ram[ACC];
      break;
    case at_dptr:
      p = xram + ram[DPL] + ram[DPH]*BYTE_MAX;
      break;
    case at_r0:
    case at_r1:
      if (cpu_instr_tkn[op][INSTR_TKN_INSTR]==movx) 
	p = xram + ram[P2]*BYTE_MAX + *reg[**index - at_r0];
      else
	p = &atram(*reg[**index - at_r0]);
      break;
    case a:
      p = ram + ACC;
      break;
    case c: /* bit addr of carry */
      p = ram + PSW; bp = carry;
      break;
    case dptr:
      p = ram + DPL;
      break;
    case r0: case r1: case r2: case r3:
    case r4: case r5: case r6: case r7:
      p = reg[**index - r0];
      break;
    default:
      assert(TRUE);
      break;
    }
  /* if token is constant, it will have be encoded in memory, adv codeptr
   */
  if (isConstToken(**index)) ++(*code); *index += 2;
  if (bparam) *bparam = bp; if (param) *param = p; /* don't assign if NULL */
  return TRUE;
}

/* step() is the master function to update the registers and memory 
 * from the execution of the opcode at memory[pc]
 */
void step(void)
{
  int x, y, *src = NULL, *dst = NULL, bsrc = UNDEF, bdst = UNDEF, addr = 0,
      op = memory[pc], opcode = cpu_instr_tkn[op][INSTR_TKN_INSTR],
      *code = memory + pc + 1;
  const int *index = cpu_instr_tkn[op] + INSTR_TKN_PARAM;
  
  /* value of pc during instr execution is pc of next instr
   * return immediately if pc has overflowed (let sim register error)
   */
  pc += cpu_instr_tkn[memory[pc]][INSTR_TKN_BYTES];
  if (pc>MEMORY_MAX) return;

  /* Look for up to 3 parameters in opcdoe. getParam will 
   * return 0 if no more paramaters to be found.
   * 1st param dest reg or memory, 2nd param source or addr, 
   * 3rd param always address
   */
  getParam(op, &index, &code, &dst, &bdst, &addr) &&
  getParam(op, &index, &code, &src, &bsrc, &addr) &&
  getParam(op, &index, &code, NULL, NULL,  &addr);

  /* calls to getparam will set dst, src registers or memory locations
   * plus any address. switch statment acts on these values
   */
  switch (opcode)
    {
    case acdup: case acall: /* acall addr_11 */
      pushStack(getLow(pc));
      pushStack(getHigh(pc));
    case ajdup: case ajmp: /* ajmp addr_11 */
      pc = (pc & 0xF800) + addr;
      break;
    case add:  /* add  dst, src */
    case addc: /* addc dst, src */
      doAdd(*src, (opcode - add) ? TRUE : FALSE);
      break;
    case anl: /* anl dst, src */
      if (bdst != UNDEF) /* anl dst.bdst, src.bsrc */
	setBit((*dst & bdst) && ((bsrc<0) ? !(*src & (-bsrc)) : *src & bsrc), dst, bdst);
      else
	*dst &= *src;
      break;
    case cjne: /* cjne dst, src, addr */
      setC(*dst <  *src);
      if (*dst != *src) relJmp(pc, addr);
      break;
    case clr: /* clr dst */
      if (bdst != UNDEF) /* clr dst.bst */
	setBit(0, dst, bdst); 
      else 
	*dst = 0;
      break;
    case cpl: /* cpl dst */
      if (bdst != UNDEF) /* cpl dst.bst */
	setBit(((*dst & bdst) != 0) ^ 1, dst, bdst);
      else
	*dst ^= BYTE_MASK;
      break;
    case da: /* da a */
      if ((ram[ACC] & LO_NYBLE)>0x09 || (ram[PSW] | auxc)>0) doAdd(0x06, FALSE);
      if ((ram[ACC] & HI_NYBLE)>0x90 || getC())              doAdd(0x60, FALSE);
      break;
    case dec: /* dec dst */
      dec(*dst);
      break;
    case divab: /* div ab */
      x = ram[ACC]; y = ram[B];
      if (y)
	{
	  ram[ACC] = x/y; ram[B] = x%y;
	}
      setPSW(!y, ov); setC(0);
      break;
    case djnz: /* djnz dst, rel_addr */
      if (dec(*dst)) relJmp(pc, addr);
      break;
    case inc: /* inc dst */
      inc(*dst);
      if (dst==(ram + DPL) && !(*dst)) inc(ram[DPH]); /* inc dptr */
      break;
    case jb: /* jb dst.bdst, rel_addr */
      if (*dst & bdst) relJmp(pc, addr);
      break;
    case jbc: /* jbc dst.bdst, rel_addr */
      if (*dst & bdst)
	{
	  setBit(0, dst, bdst); relJmp(pc, addr); 
	}
      break;
    case jc: /* jc rel_addr */
      if (getC()) relJmp(pc, addr);
      break;
    case jnb: /* jnb dst.bdst, rel_addr */
      if (!(*dst & bdst)) relJmp(pc, addr);
      break;
    case jnc: /* jnc rel_addr */
      if (!getC()) relJmp(pc, addr);
      break;
    case jnz: /* jnz rel_addr */
      if (ram[ACC]) relJmp(pc, addr);
      break;
    case jz: /* jz rel_addr */
      if (!(ram[ACC])) relJmp(pc, addr);
      break;
    case lcall: /* lcall addr_16 */
      pushStack(getLow(pc));
      pushStack(getHigh(pc));
    case jmp:  /* jmp @a+dptr */
    case ljmp: /* ljmp addr_16 */
      pc = addr;
      break;
    case movc: /* movc src, addr_16 */
      src = memory + addr;
    case movx: case mov: /* mov dst, src */
      if (op == 0x85)
	*src = *dst;      /* for mov addr_8, addr_8 src & dst are reversed */
      else if (bdst != UNDEF) /* mov dst.bdst, src.bsrc */
	setBit(*src & bsrc, dst, bdst);
      else 
	{
	  if (op == 0x90) ram[DPH] = *(src++); /* mov dptr, #data_16 */
	  *dst = *src;
	}
      break;
    case mul: /* mul ab */
      x = ram[ACC] * (ram[B]);
      ram[ACC] = getLow(x); ram[B] = getHigh(x);
      setPSW(x >= BYTE_MAX, ov);
      setC(0);
      break;
    case nop: case rsrvd:
      break;
    case orl: /* orl dst, src */
      if (bdst != UNDEF) /* orl dst.bdst, src.bsrc */
	setBit((*dst & bdst) || ((bsrc<0) ? !(*src & (-bsrc)) : *src & bsrc), dst, bdst);
      else
	*dst |= *src;
      break;
    case pop: /* pop addr_8 */
      *dst = popStack();
      break;
    case push:  /* push addr_8 */
      pushStack(*dst);
      break;
    case reti: case ret: /* reti, OR ret */
      pc = popStack()*BYTE_MAX + popStack();
      break;
    case rl: /* rl a */
      ram[ACC] = ((ram[ACC])*2 + (ram[ACC]>=BIT7_MASK)) & BYTE_MASK;
      break;
    case rlc: /* rlc a */
      ram[ACC] = ram[ACC]*2 + getC();
      setC(ram[ACC]>=BYTE_MAX);
      ram[ACC] &= BYTE_MASK;
      break;
    case rr: /* rr a */
      ram[ACC] = (ram[ACC])/2 + (ram[ACC] & BIT0_MASK)*BIT7_MASK;
      break;
    case rrc: /* rrc a */
      x = getC();
      setC(ram[ACC] & BIT0_MASK);
      ram[ACC] |= 2*x*BIT7_MASK;
      ram[ACC] /= 2;
      break;
    case setb: /* setb dst.bdst */
      setBit(1, dst, bdst);
      break;
    case sjmp: /* sjmp rel_addr */
      relJmp(pc, addr);
      break;
    case subb: /* subb a, dst */
      doSub(*src);
      break;
    case swap: /* swap a */
      ram[ACC] = (ram[ACC] & LO_NYBLE)*0x10 + ram[ACC]/0x10;
      break;
    case xch: /* xch a, src */
      x = *src; *src = *dst; *dst = x;
      break;
    case xchd: /* xchd a, src */
      x = *src & LO_NYBLE; 
      *src = (*src & HI_NYBLE) + (*dst & LO_NYBLE);
      *dst = (*dst & HI_NYBLE) + x;
      break;
    case xrl: /* xrl dst, src */
      if (bdst != UNDEF) /* xrl dst.bdst, src.bsrc */
	setBit(((*dst & bdst) != 0) ^ (((bsrc<0) ? !(*src & (-bsrc)) : *src & bsrc) != 0), dst, bdst);
      else
	*dst ^= *src;
      break;
    default:
      assert(TRUE);
      break;
    }
  updatePSW();
}

/* getRegister will return the address to the name of the register given it
 * if *bit not UNDEF, register is one bit in length
 */
int *getRegister(str_storage name, int *bit, int* bytes)
{
  int index, addr;
  const str_storage *ret = bsearch(name, tokens, tokens_length, sizeof(str_storage), &cmpstr);
  if (!ret)
    {
      *bytes = 1; *bit = UNDEF; index = 0;
      while (def_labels[index].value != UNDEF && strcmp(name, def_labels[index].name)) ++index;
      addr = def_labels[index].value;
      return (addr != UNDEF && addr > 0x7F && addr < 0x100) ? ram + addr : NULL;
    }
  else
    {
      *bit = UNDEF; *bytes = isRegister_table[ret - tokens];
      index = ret - tokens + PROC_TOKEN;
      switch (index)
	{
	case dptr: return ram + DPL + BYTE_MAX*ram[DPH]; break;
	case pc_reg: return &pc;   break;
	case r0: case r1: case r2: case r3:
	case r4: case r5: case r6: case r7: 
	  return reg[index-r0];    break;
	case a:  return ram + ACC; break;
	case c:  *bit = carry;     return ram + PSW;   break;
	default: return NULL;      break;
	}
    }
}

/* getMemory will return the value of the memory location addr
 * m allows to access to internal RAM, external RAM and external ROM
 */
int *getMemory(int addr, char m)
{
  int b, *mptr = NULL;
  if (m == '\0') m = 'c';
  switch (m)
    {
    case 'd': 
      if (addr>=BYTE_MAX) return NULL;
      mptr = ram + addr;
      break;
    case 'i': 
      if (addr>=BYTE_MAX) return NULL;
      mptr = &atram(addr);
      break;
    case 'x':
      if (addr>=MEMORY_MAX) return NULL;
      mptr = xram + addr;
      break;
    case 'c':
      if (addr>=MEMORY_MAX) return NULL;
      mptr = memory + addr;
      break;
    case 'b':
      bitAddr(addr, mptr, b);
      break;
    default:
      break;
    }
  return mptr;
}

/***************************************
 * 
 * Complete list of 8051 instructions

0x11: acall addr_11
0x31: acall addr_11
0x51: acall addr_11
0x71: acall addr_11
0x91: acall addr_11
0xB1: acall addr_11
0xD1: acall addr_11
0xF1: acall addr_11
0x25: add a, addr_8
0x24: add a, #data_8
0x26: add a, @r0
0x27: add a, @r1
0x28: add a, r0
0x29: add a, r1
0x2A: add a, r2
0x2B: add a, r3
0x2C: add a, r4
0x2D: add a, r5
0x2E: add a, r6
0x2F: add a, r7
0x35: addc a, addr_8
0x34: addc a, #data_8
0x36: addc a, @r0
0x37: addc a, @r1
0x38: addc a, r0
0x39: addc a, r1
0x3A: addc a, r2
0x3B: addc a, r3
0x3C: addc a, r4
0x3D: addc a, r5
0x3E: addc a, r6
0x3F: addc a, r7
0x01: ajmp addr_11
0x21: ajmp addr_11
0x41: ajmp addr_11
0x61: ajmp addr_11
0x81: ajmp addr_11
0xA1: ajmp addr_11
0xC1: ajmp addr_11
0xE1: ajmp addr_11
0x55: anl a, addr_8
0x54: anl a, #data_8
0x52: anl addr_8, a
0x53: anl addr_8, #data_8
0x56: anl a, @r0
0x57: anl a, @r1
0x58: anl a, r0
0x59: anl a, r1
0x5A: anl a, r2
0x5B: anl a, r3
0x5C: anl a, r4
0x5D: anl a, r5
0x5E: anl a, r6
0x5F: anl a, r7
0x82: anl c, bit_addr
0xB0: anl c, /bit_addr
0xB5: cjne a, addr_8, rel_addr
0xB4: cjne a, #data_8, rel_addr
0xB6: cjne @r0, #data_8, rel_addr
0xB7: cjne @r1, #data_8, rel_addr
0xB8: cjne r0, #data_8, rel_addr
0xB9: cjne r1, #data_8, rel_addr
0xBA: cjne r2, #data_8, rel_addr
0xBB: cjne r3, #data_8, rel_addr
0xBC: cjne r4, #data_8, rel_addr
0xBD: cjne r5, #data_8, rel_addr
0xBE: cjne r6, #data_8, rel_addr
0xBF: cjne r7, #data_8, rel_addr
0xE4: clr a
0xC2: clr bit_addr
0xC3: clr c
0xF4: cpl a
0xB2: cpl bit_addr
0xB3: cpl c
0xD4: da a
0x14: dec a
0x15: dec addr_8
0x16: dec @r0
0x17: dec @r1
0x18: dec r0
0x19: dec r1
0x1A: dec r2
0x1B: dec r3
0x1C: dec r4
0x1D: dec r5
0x1E: dec r6
0x1F: dec r7
0x84: div ab
0xD5: djnz addr_8, rel_addr
0xD8: djnz r0, rel_addr
0xD9: djnz r1, rel_addr
0xDA: djnz r2, rel_addr
0xDB: djnz r3, rel_addr
0xDC: djnz r4, rel_addr
0xDD: djnz r5, rel_addr
0xDE: djnz r6, rel_addr
0xDF: djnz r7, rel_addr
0x04: inc a
0x05: inc addr_8
0xA3: inc dptr
0x06: inc @r0
0x07: inc @r1
0x08: inc r0
0x09: inc r1
0x0A: inc r2
0x0B: inc r3
0x0C: inc r4
0x0D: inc r5
0x0E: inc r6
0x0F: inc r7
0x20: jb bit_addr, rel_addr
0x10: jbc bit_addr, rel_addr
0x40: jc rel_addr
0x73: jmp @a+dptr
0x30: jnb bit_addr, rel_addr
0x50: jnc rel_addr
0x70: jnz rel_addr
0x60: jz rel_addr
0x12: lcall addr_16
0x02: ljmp addr_16
0xE5: mov a, addr_8
0x74: mov a, #data_8
0xF5: mov addr_8, a
0x85: mov addr_8, addr_8
0x75: mov addr_8, #data_8
0x88: mov addr_8, r0
0x86: mov addr_8, @r0
0x87: mov addr_8, @r1
0x89: mov addr_8, r1
0x8A: mov addr_8, r2
0x8B: mov addr_8, r3
0x8C: mov addr_8, r4
0x8D: mov addr_8, r5
0x8E: mov addr_8, r6
0x8F: mov addr_8, r7
0xE6: mov a, @r0
0xE7: mov a, @r1
0xE8: mov a, r0
0xE9: mov a, r1
0xEA: mov a, r2
0xEB: mov a, r3
0xEC: mov a, r4
0xED: mov a, r5
0xEE: mov a, r6
0xEF: mov a, r7
0x92: mov bit_addr, C
0x93: movc a, @a+dptr
0x83: movc a,@a+pc
0xA2: mov c, bit_addr
0x90: mov dptr, #data_16
0xF6: mov @r0, a
0xA6: mov @r0, addr_8
0x76: mov @r0, #data_8
0xF7: mov @r1, a
0xA7: mov @r1, addr_8
0x77: mov @r1, #data_8
0xF8: mov r0, a
0xA8: mov r0, addr_8
0x78: mov r0, #data_8
0xF9: mov r1, a
0xA9: mov r1, addr_8
0x79: mov r1, #data_8
0xFA: mov r2, a
0xAA: mov r2, addr_8
0x7A: mov r2, #data_8
0xFB: mov r3, a
0xAB: mov r3, addr_8
0x7B: mov r3, #data_8
0xFC: mov r4, a
0xAC: mov r4, addr_8
0x7C: mov r4, #data_8
0xFD: mov r5, a
0xAD: mov r5, addr_8
0x7D: mov r5, #data_8
0xFE: mov r6, a
0xAE: mov r6, addr_8
0x7E: mov r6, #data_8
0xFF: mov r7, a
0xAF: mov r7, addr_8
0x7F: mov r7, #data_8
0xF0: movx @dptr, a
0xE0: movx a, @dptr
0xF0: movx @dptr, a
0xF2: movx @r0, a
0xF3: movx @r1, a
0xE2: movx a, @r0
0xE3: movx a, @r1
0xA4: mul ab
0x00: nop
0x45: orl a, addr_8
0x44: orl a, #data_8
0x42: orl addr_8, a
0x43: orl addr_8, #data_8
0x46: orl a, @r0
0x47: orl a, @r1
0x48: orl a, r0
0x49: orl a, r1
0x4A: orl a, r2
0x4B: orl a, r3
0x4C: orl a, r4
0x4D: orl a, r5
0x4E: orl a, r6
0x4F: orl a, r7
0x72: orl c, bit_addr
0xA0: orl c, /bit_addr
0xD0: pop addr_8
0xC0: push addr_8
0xA5: reserved opcode
0x22: ret
0x32: reti
0x23: rl a
0x33: rlc a
0x03: rr a
0x13: rrc a
0xD2: setb bit_addr
0xD3: setb c
0x80: sjmp rel_addr
0x95: subb a, addr_8
0x94: subb a, #data_8
0x96: subb a, @r0
0x97: subb a, @r1
0x98: subb a, r0
0x99: subb a, r1
0x9A: subb a, r2
0x9B: subb a, r3
0x9C: subb a, r4
0x9D: subb a, r5
0x9E: subb a, r6
0x9F: subb a, r7
0xC4: swap a
0xC5: xch a, addr_8
0xC6: xch a, @r0
0xC7: xch a, @r1
0xC8: xch a, r0
0xC9: xch a, r1
0xCA: xch a, r2
0xCB: xch a, r3
0xCC: xch a, r4
0xCD: xch a, r5
0xCE: xch a, r6
0xCF: xch a, r7
0xD6: xchd a, @r0
0xD7: xchd a, @r1
0x65: xrl a, addr_8
0x64: xrl a, #data_8
0x62: xrl addr_8, a
0x63: xrl addr_8, #data_8
0x66: xrl a, @r0
0x67: xrl a, @r1
0x68: xrl a, r0
0x69: xrl a, r1
0x6A: xrl a, r2
0x6B: xrl a, r3
0x6C: xrl a, r4
0x6D: xrl a, r5
0x6E: xrl a, r6
0x6F: xrl a, r7
 * 
 * 
 ***************************************/


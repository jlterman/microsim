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
#define SIM_LOCAL

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "asm.h"
#include "front.h"
#include "back.h"
#include "proc.h"

#define bitAddr(baddr, addr, bit) \
{ \
  bit = 1<<((baddr) % 8); \
  addr = ((baddr)>0x7F) ? (baddr) & 0xF8 : 0x20 + (baddr)/8; \
}

#define relJmp(pc, rel) \
{ \
  pc += cpu_instr_tkn[memory[pc]][INSTR_TKN_BYTES] + (rel) - ((rel)>SGN_BYTE_MAX) ? 0x100 : 0; \
  return; \
}

#define push(x) *(++sp) = (x)
#define pop() (*(sp--))

#define setC() (*psw |= 0x80)
#define clrC() (*psw &= 0x7F)
#define getC() (*psw > 0x80)

#define setPSW(x) (*psw |= (x))
#define clrPSW(x) (*psw &= (0xFF - (x)))
#define carry 128
#define auxc   64
#define rs1    16
#define rs0     8
#define ov      4
#define prty    1

static int *acc, *b, *sp, *psw, *reg0, *reg1, *reg2, *reg3, *reg4, *reg5, *reg6, *reg7, *dpl, *dph;
static int ram[BYTE_MAX+1] = { 0 };
static int xram[MEMORY_MAX] = { 0 };

void reset()
{
  acc = memory + getLabelValue("acc");
  b   = memory + getLabelValue("b");
  psw = memory + getLabelValue("psw");
  sp  = memory + getLabelValue("sp");
  dpl = memory + getLabelValue("dpl");
  dph = memory + getLabelValue("dph");
  reg0  = ram + 0;
  reg1  = ram + 1;
  reg2  = ram + 2;
  reg3  = ram + 3;
  reg4  = ram + 4;
  reg5  = ram + 5;
  reg6  = ram + 6;
  reg7  = ram + 7;
}

static void doAdd(int value, int carryFlag)
{
  int ac, nc;

  ac = ((*acc & 0x0F) + (value & 0x0F) + getC()*carryFlag) > 0xF;
  if (ac) setPSW(auxc); else clrPSW(auxc);

  nc = (*acc + value + (getC()*carryFlag)) > 0xFF;
  ac = ((*acc & 0x7F) + (value & 0x7F) + getC()*carryFlag) > 0x7F;
  if (ac ^ nc) setPSW(ov); else clrPSW(ov);

  if (nc) setC(); else clrC();
  *acc = (*acc + value + getC()*carryFlag) & 0xFF;
}

static void doSub(int value)
{
  int ac, nc;

  ac = ((*acc & 0x0F) - (value & 0x0F) - getC()) < 0;
  if (ac) setPSW(auxc); else clrPSW(auxc);

  nc = (*acc - value - getC()) < 0;
  ac = ((*acc & 0x7F) - (value & 0x7F) - getC()) < 0;
  if (ac ^ nc) setPSW(ov); else clrPSW(ov);

  if (nc) setC(); else clrC();
  *acc = (*acc - value - getC()) & 0xFF;
}

void doStep()
{
  int addr, c, bit, x, y;

  switch (memory[pc])
    {
    case 0x00: /* nop */
      break;
    case 0x01: /* ajmp addr */
      pc = memory[pc+1];
      return;
      break;
    case 0x02: /* ljmp addr */
      pc = memory[pc+1]*0x100 + memory[pc+2];
      return;
      break;
    case 0x03: /* rr a */
      *acc = *acc/2 + 0x80 * (*acc % 2);
      break;
    case 0x04: /* inc a */
      *acc = getLow(*acc+1);
      break;
    case 0x05: /* inc addr */
      addr = memory[pc+1];
      ram[addr] = getLow(ram[addr]+1);
      break;
    case 0x06: /* inc @r0 */
      addr = *reg0;
      ram[addr] = getLow(ram[addr]+1);
      break;
    case 0x07: /* inc @r1 */
      addr = *reg1;
      ram[addr] = getLow(ram[addr]+1);
      break;
    case 0x08: /* inc r0 */
      *reg0 = getLow(*reg0+1);
      break;
    case 0x09: /* inc r1 */
      *reg1 = getLow(*reg1+1);
      break;
    case 0x0A: /* inc r2 */
      *reg2 = getLow(*reg2+1);
      break;
    case 0x0B: /* inc r3 */
      *reg3 = getLow(*reg3+1);
      break;
    case 0x0C: /* inc r4 */
      *reg4 = getLow(*reg4+1);
      break;
    case 0x0D: /* inc r5 */
      *reg5 = getLow(*reg5+1);
      break;
    case 0x0E: /* inc r6 */
      *reg6 = getLow(*reg6+1);
      break;
    case 0x0F: /* inc r7 */
      *reg7 = getLow(*reg7+1);
      break;
    case 0x10: /* jbc bit_addr, addr */
      bitAddr(memory[pc+1], addr, bit);
      if (ram[addr] & bit)
	{
	  ram[addr] &= 0xFF - bit;
	  relJmp(pc, memory[pc+2]);
	}
      break;
    case 0x11: /* acall addr */
      pc += 2;
      push(getLow(pc));
      push(getHigh(pc));

      pc =  memory[pc+1];
      return;
      break;
    case 0x12: /* lcall addr */
      pc += 3;
      push(getLow(pc));
      push(getHigh(pc));

      pc = memory[pc+1]*0x100 + memory[pc+2];
      return;
      break;
    case 0x13: /* rrc a */
      c = *acc & 1;
      *acc /= 2;
      *acc |= getC()*0x80;
      (c) ? setC() : clrC();
      break;
    case 0x14: /* dec a */
      *acc = getLow(*acc-1);
      break;
    case 0x15: /* dec addr */
      addr = memory[pc+1];
      ram[addr] = getLow(ram[addr]-1);
      break;
    case 0x16: /* dec @r0 */
      addr = *reg0;
      ram[addr] = getLow(ram[addr]-1);
      break;
    case 0x17: /* dec @r1 */
      addr = *reg1;
      ram[addr] = getLow(ram[addr]-1);
      break;
    case 0x18: /* dec r0 */
      *reg0 = getLow(*reg0-1);
      break;
    case 0x19: /* dec r1 */
      *reg1 = getLow(*reg1-1);
      break;
    case 0x1A: /* dec r2 */
      *reg2 = getLow(*reg2-1);
      break;
    case 0x1B: /* dec r3 */
      *reg3 = getLow(*reg3-1);
      break;
    case 0x1C: /* dec r4 */
      *reg4 = getLow(*reg4-1);
      break;
    case 0x1D: /* dec r5 */
      *reg5 = getLow(*reg5-1);
      break;
    case 0x1E: /* dec r6 */
      *reg6 = getLow(*reg6-1);
      break;
    case 0x1F: /* dec r7 */
      *reg7 = getLow(*reg7-1);
      break;
    case 0x20: /* jb bit_addr, addr */
      bitAddr(memory[pc+1], addr, bit);
      if (ram[addr] & bit) relJmp(pc, memory[pc+2]);
      break;      
    case 0x21: /* ajmp addr */
      pc = 0x100 + memory[pc+1];
      return;
      break;
    case 0x22: /* ret */
      y = pop();
      x = pop();
      pc = y*0x100 + x;
      return;
      break;
    case 0x23: /* rl a */
      c = *acc & 0x80;
      *acc = (*acc & 0x7F)*2;
      *acc += (c!=0);
      break;
    case 0x24: /* add a, #data */
      doAdd(memory[pc+1], FALSE);
      break;
    case 0x25: /* add a, addr */
      addr = memory[pc+1];
      doAdd(ram[addr], FALSE);
      break;
    case 0x26: /* add a, @r0 */
      doAdd(ram[*reg0], FALSE);
      break;
    case 0x27: /* add a, @r1 */
      doAdd(ram[*reg1], FALSE);
      break;
    case 0x28: /* add a, r0 */
      doAdd(*reg0, FALSE);
      break;
    case 0x29: /* add a, r1 */
      doAdd(*reg1, FALSE);
      break;
    case 0x2A: /* add a, r2 */
      doAdd(*reg2, FALSE);
      break;
    case 0x2B: /* add a, r3 */
      doAdd(*reg3, FALSE);
      break;
    case 0x2C: /* add a, r4 */
      doAdd(*reg4, FALSE);
      break;
    case 0x2D: /* add a, r5 */
      doAdd(*reg5, FALSE);
      break;
    case 0x2E: /* add a, r6 */
      doAdd(*reg6, FALSE);
      break;
    case 0x2F: /* add a, r7 */
      doAdd(*reg7, FALSE);
      break;
    case 0x30: /* jnb bit_addr, addr */
      bitAddr(memory[pc+1], addr, bit);
      if (!(ram[addr] & bit)) relJmp(pc, memory[pc+2]);
      break;      
    case 0x31: /* acall addcr */
      pc += 2;
      push(getLow(pc));
      push(getHigh(pc));

      pc = 0x100 + memory[pc+1];
      return;
      break;
    case 0x32: /* reti */
      y = pop();
      x = pop();
      pc = y*0x100 + x;
      return;
      break;
    case 0x33: /* rlc a */
      c = *acc & 0x80;
      *acc = (*acc & 0x7F)*2;
      *acc += (c!=0);

      *acc = *acc*2;
      *acc |= getC();
      if (*acc & 0x100)
	{
	  *acc &= 0xFF; setC();
	}
      else
	clrC();
      break;
    case 0x34: /* addc a, #data */
      doAdd(ram[pc+1], TRUE);
      break;
    case 0x35: /* addc a, addr */
      addr = memory[pc+1];
      doAdd(ram[addr], TRUE);
      break;
    case 0x36: /* addc a, @r0 */
      doAdd(ram[*reg0], TRUE);
      break;
    case 0x37: /* addc a, @r1 */
      doAdd(ram[*reg1], TRUE);
      break;
    case 0x38: /* addc a, r0 */
      doAdd(*reg0, TRUE);
      break;
    case 0x39: /* addc a, r1 */
      doAdd(*reg1, TRUE);
      break;
    case 0x3A: /* addc a, r2 */
      doAdd(*reg2, TRUE);
      break;
    case 0x3B: /* addc a, r3 */
      doAdd(*reg3, TRUE);
      break;
    case 0x3C: /* addc a, r4 */
      doAdd(*reg4, TRUE);
      break;
    case 0x3D: /* addc a, r5 */
      doAdd(*reg5, TRUE);
      break;
    case 0x3E: /* addc a, r6 */
      doAdd(*reg6, TRUE);
      break;
    case 0x3F: /* addc a, r7 */
      doAdd(*reg7, TRUE);
      break;
    case 0x40: /* jc addr */
      if (getC()) relJmp(pc, memory[pc+1]);
      break;      
    case 0x41: /* ajmp addr */
      pc = 0x200 + memory[pc+1];
      return;
      break;
    case 0x42: /* orl addr, a */
      addr = memory[pc+1];
      ram[addr] |= *acc;
      break;
    case 0x43: /* orl addr, #data */
      addr = memory[pc+1];
      ram[addr] |= ram[pc+2];
      break;
    case 0x44: /* orl a, #data */
      *acc |= ram[pc+1];
      break;
    case 0x45: /* orl a, addr */
      addr = memory[pc+1];
      *acc |= ram[addr];
      break;
    case 0x46: /* orl a, @r0 */
      *acc |= ram[*reg0];
      break;
    case 0x47: /* orl a, @r1 */
      *acc |= ram[*reg1];
      break;
    case 0x48: /* orl a, r0 */
      *acc |= *reg0;
      break;
    case 0x49: /* orl a, r1 */
      *acc |= *reg1;
      break;
    case 0x4A: /* orl a, r2 */
      *acc |= *reg2;
      break;
    case 0x4B: /* orl a, r3 */
      *acc |= *reg3;
      break;
    case 0x4C: /* orl a, r4 */
      *acc |= *reg4;
      break;
    case 0x4D: /* orl a, r5 */
      *acc |= *reg5;
      break;
    case 0x4E: /* orl a, r6 */
      *acc |= *reg6;
      break;
    case 0x4F: /* orl a, r7 */
      *acc |= *reg7;
      break;
    case 0x50: /* jnc addr */
      if (!getC()) relJmp(pc, memory[pc+1]);
      break;      
    case 0x51: /* acall addr */
      pc += 2;
      push(getLow(pc));
      push(getHigh(pc));

      pc = 0x200 + memory[pc+1];
      return;
      break;
    case 0x52: /* anl addr, a */
      addr = memory[pc+1];
      ram[addr] &= *acc;
      break;
    case 0x53: /* anl addr, #data */
      addr = memory[pc+1];
      ram[addr] &= ram[pc+2];
      break;
    case 0x54: /* anl a, #data */
      *acc &= ram[pc+1];
      break;
    case 0x55: /* anl a, addr */
      addr = memory[pc+1];
      *acc &= ram[addr];
      break;
    case 0x56: /* anl a, @r0 */
      *acc &= ram[*reg0];
      break;
    case 0x57: /* anl a, @r1 */
      *acc &= ram[*reg1];
      break;
    case 0x58: /* anl a, r0 */
      *acc &= *reg0;
      break;
    case 0x59: /* anl a, r1 */
      *acc &= *reg1;
      break;
    case 0x5A: /* anl a, r2 */
      *acc &= *reg2;
      break;
    case 0x5B: /* anl a, r3 */
      *acc &= *reg3;
      break;
    case 0x5C: /* anl a, r4 */
      *acc &= *reg4;
      break;
    case 0x5D: /* anl a, r5 */
      *acc &= *reg5;
      break;
    case 0x5E: /* anl a, r6 */
      *acc &= *reg6;
      break;
    case 0x5F: /* anl a, r7 */
      *acc &= *reg7;
      break;
    case 0x60: /* jz addr */
      if (*acc == 0) relJmp(pc, memory[pc+1]);
      break;      
    case 0x61: /* ajmp addr */
      pc = 0x300 + memory[pc+1];
      return;
      break;
    case 0x62: /* xrl addr, a */
      addr = memory[pc+1];
      ram[addr] ^= *acc;
      break;
    case 0x63: /* xrl addr, #data */
      addr = memory[pc+1];
      ram[addr] ^= ram[pc+2];
      break;
    case 0x64: /* xrl a, #data */
      *acc ^= ram[pc+1];
      break;
    case 0x65: /* xrl a, addr */
      addr = memory[pc+1];
      *acc ^= ram[addr];
      break;
    case 0x66: /* xrl a, @r0 */
      *acc ^= ram[*reg0];
      break;
    case 0x67: /* xrl a, @r1 */
      *acc ^= ram[*reg1];
      break;
    case 0x68: /* xrl a, r0 */
      *acc ^= *reg0;
      break;
    case 0x69: /* xrl a, r1 */
      *acc ^= *reg1;
      break;
    case 0x6A: /* xrl a, r2 */
      *acc ^= *reg2;
      break;
    case 0x6B: /* xrl a, r3 */
      *acc ^= *reg3;
      break;
    case 0x6C: /* xrl a, r4 */
      *acc ^= *reg4;
      break;
    case 0x6D: /* xrl a, r5 */
      *acc ^= *reg5;
      break;
    case 0x6E: /* xrl a, r6 */
      *acc ^= *reg6;
      break;
    case 0x6F: /* xrl a, r7 */
      *acc ^= *reg7;
      break;
    case 0x70: /* jnz addr */
      if (*acc) relJmp(pc, memory[pc+1]);
      break;      
    case 0x71: /* acall addr */
      pc += 2;
      push(getLow(pc));
      push(getHigh(pc));

      pc = 0x300 + memory[pc+1];
      return;
      break;
    case 0x72: /* orl c, bit_addr */
      bitAddr(memory[pc+1], addr, bit);
      if ( (ram[addr] & bit) || getC() ) setC(); else clrC();
      break;
    case 0x73: /* jmp @a+dptr */
      pc = *acc + *dpl + *dph*0x100;
      return;
      break;
    case 0x74: /* mov a, #data */
      *acc = memory[pc+1];
      break;
    case 0x75: /* mov addr, #data */
      addr = memory[pc+1];
      ram[addr] = memory[pc+2];
      break;
    case 0x76: /* mov @r0, #data */
      ram[*reg0] = memory[pc+1];
      break;
    case 0x77: /* mov @r1, #data */
      ram[*reg1] = memory[pc+1];
      break;
    case 0x78: /* mov r0, #data */
      *reg0 = memory[pc+1];
      break;
    case 0x79: /* mov r1, #data */
      *reg1 = memory[pc+1];
      break;
    case 0x7A: /* mov r2, #data */
      *reg2 = memory[pc+1];
      break;
    case 0x7B: /* mov r3, #data */
      *reg3 = memory[pc+1];
      break;
    case 0x7C: /* mov r4, #data */
      *reg4 = memory[pc+1];
      break;
    case 0x7D: /* mov r5, #data */
      *reg5 = memory[pc+1];
      break;
    case 0x7E: /* mov r6, #data */
      *reg6 = memory[pc+1];
      break;
    case 0x7F: /* mov r7, #data */
      *reg7 = memory[pc+1];
      break;
    case 0x80: /* sjmp addr */
      relJmp(pc, memory[pc+1]);
      break;      
    case 0x81: /* ajmp addr */
      pc = 0x400 + memory[pc+1];
      return;
      break;
    case 0x82: /* anl c, bit_addr */
      bitAddr(memory[pc+1], addr, bit);
      if ( (ram[addr] & bit) && getC() ) setC(); else clrC();
      break;
    case 0x83: /* movc a,@a+pc */
      *acc = memory[*acc + pc];
      break;
    case 0x84: /* div ab */
      if (*b)
	{
	  x = *acc; y = *b;
	  *acc = x/y; *b = x%y;
	  clrPSW(carry + ov);
	}
      else
	{
	  *acc = 0xFF; *b = 0xFF;
	  clrC();
	  setPSW(ov);
	}
      break;
    case 0x85: /* mov addr, addr */
      addr = memory[pc+2];
      ram[addr] = ram[memory[pc+1]];
      break;
    case 0x86: /* mov addr, @r0 */
      addr = memory[pc+1];
      ram[addr] = memory[*reg0];
      break;
    case 0x87: /* mov addr, @r1 */
      addr = memory[pc+1];
      ram[addr] = memory[*reg1];
      break;
    case 0x88: /* mov addr, r0 */
      addr = memory[pc+1];
      ram[addr] = *reg0;
      break;
    case 0x89: /* mov addr, r1 */
      addr = memory[pc+1];
      ram[addr] = *reg1;
      break;
    case 0x8A: /* mov addr, r2 */
      addr = memory[pc+1];
      ram[addr] = *reg2;
      break;
    case 0x8B: /* mov addr, r3 */
      addr = memory[pc+1];
      ram[addr] = *reg3;
      break;
    case 0x8C: /* mov addr, r4 */
      addr = memory[pc+1];
      ram[addr] = *reg4;
      break;
    case 0x8D: /* mov addr, r5 */
      addr = memory[pc+1];
      ram[addr] = *reg5;
      break;
    case 0x8E: /* mov addr, r6 */
      addr = memory[pc+1];
      ram[addr] = *reg6;
      break;
    case 0x8F: /* mov addr, r7 */
      addr = memory[pc+1];
      ram[addr] = *reg7;
      break;
    case 0x90: /* mov dptr, #data */
      *dph = memory[pc+1];
      *dpl = memory[pc+2];
      break;
    case 0x91: /* acall addr */
      pc += 2;
      push(getLow(pc));
      push(getHigh(pc));

      pc = 0x400 + memory[pc+1];
      return;
      break;
    case 0x92: /* mov bit_addr, C */
      bitAddr(memory[pc+1], addr, bit);
      if (getC()) 
	ram[addr] |= bit; 
      else 
	ram[addr] &= (0xFF - bit);
      break;
    case 0x93: /* movc a, @a+dptr */
      *acc = memory[*acc + *dpl + *dph*0x100];
      break;
    case 0x94: /* subb a, #data */
      doSub(ram[pc+1]);
      break;
    case 0x95: /* subb a, addr */
      addr = memory[pc+1];
      doSub(ram[addr]);
      break;
    case 0x96: /* subb a, @r0 */
      doSub(ram[*reg0]);
      break;
    case 0x97: /* subb a, @r1 */
      doSub(ram[*reg1]);
      break;
    case 0x98: /* subb a, r0 */
      doSub(*reg0);
      break;
    case 0x99: /* subb a, r1 */
      doSub(*reg1);
      break;
    case 0x9A: /* subb a, r2 */
      doSub(*reg2);
      break;
    case 0x9B: /* subb a, r3 */
      doSub(*reg3);
      break;
    case 0x9C: /* subb a, r4 */
      doSub(*reg4);
      break;
    case 0x9D: /* subb a, r5 */
      doSub(*reg5);
      break;
    case 0x9E: /* subb a, r6 */
      doSub(*reg6);
      break;
    case 0x9F: /* subb a, r7 */
      doSub(*reg7);
      break;
    case 0xA0: /* orl c, /bit_addr */
      bitAddr(memory[pc+1], addr, bit);
      if ( !(ram[addr] & bit) || getC() ) setC(); else clrC();
      break;
    case 0xA1: /* ajmp addr */
      pc = 0x500 + memory[pc+1];
      return;
      break;
    case 0xA2: /* mov c, bit_addr */
      bitAddr(memory[pc+1], addr, bit);
      if (ram[addr] & bit) setC(); else clrC();
      break;
    case 0xA3: /* inc dptr */
      if (*dpl == 0xFF)
	{
	  *dpl = 0; ++(*dph); *dph = getLow(*dph);
	}
      else
	++(*dpl);
      break;
    case 0xA4: /* mul ab */
      x = *acc * (*b);
      *acc = getLow(x); *b = getHigh(x);
      if (x > 0xFF) setPSW(ov); else clrPSW(ov);
      clrC();
      break;
    case 0xA5: /* reserved opcode */
      break;
    case 0xA6: /* mov @r0, addr */
      addr = memory[pc+1];
      memory[*reg0] = ram[addr];
      break;
    case 0xA7: /* mov @r1, addr */
      addr = memory[pc+1];
      memory[*reg1] = ram[addr];
      break;
    case 0xA8: /* mov r0, addr */
      addr = memory[pc+1];
      *reg0 = memory[addr];
      break;
    case 0xA9: /* mov r1, addr */
      addr = memory[pc+1];
      *reg1 = memory[addr];
      break;
    case 0xAA: /* mov r2, addr */
      addr = memory[pc+1];
      *reg2 = memory[addr];
      break;
    case 0xAB: /* mov r3, addr */
      addr = memory[pc+1];
      *reg3 = memory[addr];
      break;
    case 0xAC: /* mov r4, addr */
      addr = memory[pc+1];
      *reg4 = memory[addr];
      break;
    case 0xAD: /* mov r5, addr */
      addr = memory[pc+1];
      *reg5 = memory[addr];
      break;
    case 0xAE: /* mov r6, addr */
      addr = memory[pc+1];
      *reg6 = memory[addr];
      break;
    case 0xAF: /* mov r7, addr */
      addr = memory[pc+1];
      *reg7 = memory[addr];
      break;
    case 0xB0: /* anl c, /bit_addr */
      bitAddr(memory[pc+1], addr, bit);
      if ( !(ram[addr] & bit) && getC() ) setC(); else clrC();
      break;
    case 0xB1: /* acall addr */
      pc += 2;
      push(getLow(pc));
      push(getHigh(pc));

      pc = 0x500 + memory[pc+1];
      return;
      break;
    case 0xB2: /* cpl bit_addr */
       bitAddr(memory[pc+1], addr, bit);
       if (ram[addr] & bit) 
	 ram[addr] &= (0xFF - bit); 
       else 
	 ram[addr] |= bit;
       break;
    case 0xB3: /* cpl c */
      if (getC()) clrC(); else setC();
      break;
    case 0xB4: /* cjne a, #data, addr */
      if (*acc < memory[pc+1]) setC(); else clrC();
      if (*acc != memory[pc+1]) relJmp(pc, memory[pc+2]);
      break;
    case 0xB5: /* cjne a, direct_addr, addr */
      addr = memory[pc+1];
      if (*acc < ram[addr]) setC(); else clrC();
      if (*acc != ram[addr]) relJmp(pc, memory[pc+2]);
      break;
    case 0xB6: /* cjne @r0, #data, addr */
      if (*acc < ram[*reg0]) setC(); else clrC();
      if (ram[*reg0] != memory[pc+1]) relJmp(pc, memory[pc+2]);
      break;
    case 0xB7: /* cjne @r1, #data, addr */
      if (*acc < ram[*reg1]) setC(); else clrC();
      if (ram[*reg1] != memory[pc+1]) relJmp(pc, memory[pc+2]); 
    break;
    case 0xB8: /* cjne r0, #data, addr */
      if (*reg0 < memory[pc+1]) setC(); else clrC();
      if (*reg0 != memory[pc+1]) relJmp(pc, memory[pc+2]);
      break;
    case 0xB9: /* cjne r1, #data, addr */
      if (*reg1 < memory[pc+1]) setC(); else clrC();
      if (*reg1 != memory[pc+1]) relJmp(pc, memory[pc+2]);
      break;
    case 0xBA: /* cjne r2, #data, addr */
      if (*reg2 < memory[pc+1]) setC(); else clrC();
      if (*reg2 != memory[pc+1]) relJmp(pc, memory[pc+2]);
      break;
    case 0xBB: /* cjne r3, #data, addr */
      if (*reg3 < memory[pc+1]) setC(); else clrC();
      if (*reg3 != memory[pc+1]) relJmp(pc, memory[pc+2]);
      break;
    case 0xBC: /* cjne r4, #data, addr */
      if (*reg4 < memory[pc+1]) setC(); else clrC();
      if (*reg4 != memory[pc+1]) relJmp(pc, memory[pc+2]);
      break;
    case 0xBD: /* cjne r5, #data, addr */
      if (*reg5 < memory[pc+1]) setC(); else clrC();
      if (*reg5 != memory[pc+1]) relJmp(pc, memory[pc+2]);
      break;
    case 0xBE: /* cjne r6, #data, addr */
      if (*reg6 < memory[pc+1]) setC(); else clrC();
      if (*reg6 != memory[pc+1]) relJmp(pc, memory[pc+2]);
      break;
    case 0xBF: /* cjne r7, #data, addr */
      if (*reg7 < memory[pc+1]) setC(); else clrC();
      if (*reg7 != memory[pc+1]) relJmp(pc, memory[pc+2]);
      break;
    case 0xC0: /* push addr */
      push(memory[pc+1]);
      break;
    case 0xC1: /* ajmp addr */
      pc = 0x600 + memory[pc+1];
      return;
      break;
    case 0xC2: /* clr bit_addr */
       bitAddr(memory[pc+1], addr, bit);
       ram[addr] &= (0xFF - bit); 
       break;
    case 0xC3: /* clr c */
      clrC();
      break;
    case 0xC4: /* swap a */
      x = *acc & 0xF0;
      *acc = (*acc & 0x0F)*16 + x/16;
      break;
    case 0xC5: /* xch a, addr */
      addr = memory[pc+1];
      x = *acc; *acc = ram[addr]; ram[addr] = x;
      break;
    case 0xC6: /* xch a, @r0 */
      x = *acc; *acc = ram[*reg0]; ram[*reg0] = x;
      break;
    case 0xC7: /* xch a, @r1 */
      x = *acc; *acc = ram[*reg1]; ram[*reg1] = x;
      break;
    case 0xC8: /* xch a, r0 */
      x = *acc; *acc = *reg0; *reg0 = x;
      break;
    case 0xC9: /* xch a, r1 */
      x = *acc; *acc = *reg1; *reg1 = x;
      break;
    case 0xCA: /* xch a, r2 */
      x = *acc; *acc = *reg2; *reg2 = x;
      break;
    case 0xCB: /* xch a, r3 */
      x = *acc; *acc = *reg3; *reg3 = x;
      break;
    case 0xCC: /* xch a, r4*/
      x = *acc; *acc = *reg4; *reg4 = x;
      break;
    case 0xCD: /* xch a, r5 */
      x = *acc; *acc = *reg5; *reg5 = x;
      break;
    case 0xCE: /* xch a, r6 */
      x = *acc; *acc = *reg6; *reg6 = x;
      break;
    case 0xCF: /* xch a, r7 */
      x = *acc; *acc = *reg7; *reg7 = x;
      break;
    case 0xD0: /* pop addr */
      addr = memory[pc+1];
      ram[addr] = pop();
      break;
    case 0xD1: /* acall addr */
      pc += 2;
      push(getLow(pc));
      push(getHigh(pc));

      pc = 0x600 + memory[pc+1];
      return;
      break;
    case 0xD2: /* setb bit_addr */
       bitAddr(memory[pc+1], addr, bit);
       ram[addr] |= bit;
       break;
    case 0xD3: /* setb c */
      setC();
      break;
    case 0xD4: /* da a */
      if ( (*acc & 0x0F)>0x09 || (*psw | auxc)>0 ) doAdd(6, FALSE);
      if ( (*acc & 0xF0)>0x90 || getC()) doAdd(0x60, FALSE);
      break;
    case 0xD5: /* djnz direct_addr, addr */
      addr = memory[pc+1];
      if (--(ram[addr])) relJmp(pc, memory[pc+2]);
      break;
    case 0xD6: /* xchd a, @r0 */
      x = *acc & 0xF; *acc &= 0xF0;
      *acc |= ram[*reg0] & 0x0F; 
      ram[*reg0] &= 0xF0; ram[*reg0] |= x;
      break;
    case 0xD7: /* xchd a, @r1 */
      x = *acc & 0xF; *acc &= 0xF0;
      *acc |= ram[*reg1] & 0x0F; 
      ram[*reg1] &= 0xF0; ram[*reg1] |= x;
      break;
    case 0xD8: /* djnz r0, addr */
      if (--(*reg0)) relJmp(pc, memory[pc+1]);
      break;
    case 0xD9: /* djnz r1, addr */
      if (--(*reg1)) relJmp(pc, memory[pc+1]);
      break;
    case 0xDA: /* djnz r2, addr */
      if (--(*reg2)) relJmp(pc, memory[pc+1]);
      break;
    case 0xDB: /* djnz r3, addr */
      if (--(*reg3)) relJmp(pc, memory[pc+1]);
      break;
    case 0xDC: /* djnz r4, addr */
      if (--(*reg4)) relJmp(pc, memory[pc+1]);
      break;
    case 0xDD: /* djnz r5, addr */
      if (--(*reg5)) relJmp(pc, memory[pc+1]);
      break;
    case 0xDE: /* djnz r6, addr */
      if (--(*reg6)) relJmp(pc, memory[pc+1]);
      break;
    case 0xDF: /* djnz r7, addr */
      if (--(*reg7)) relJmp(pc, memory[pc+1]);
      break;
    case 0xE0: /* movx a, @dptr */
      addr = *dpl + *dph*0x100;
      *acc = xram[addr];
      break;
    case 0xE1: /* ajmp addr */
      pc = 0x700 + memory[pc+1];
      return;
      break;
    case 0xE2: /* movx a, @r0 */
      *acc = xram[*reg0];
      break;
    case 0xE3: /* movx a, @r1 */
      *acc = xram[*reg1];
      break;
    case 0xE4: /* clr a */
      *acc = 0;
      break;
    case 0xE5: /* mov a, addr */
      addr = memory[pc+1];
      *acc = ram[addr];
      break;
    case 0xE6: /* mov a, @r0 */
      addr = memory[pc+1];
      *acc = ram[*reg0];
      break;
    case 0xE7: /* mov a, @r1 */
      addr = memory[pc+1];
      *acc = ram[*reg1];
      break;
    case 0xE8: /* mov a, r0 */
      addr = memory[pc+1];
      *acc = *reg0;
      break;
    case 0xE9: /* mov a, r1 */
      addr = memory[pc+1];
      *acc = *reg1;
      break;
    case 0xEA: /* mov a, r2 */
      addr = memory[pc+1];
      *acc = *reg2;
      break;
    case 0xEB: /* mov a, r3 */
      addr = memory[pc+1];
      *acc = *reg3;
      break;
    case 0xEC: /* mov a, r4 */
      addr = memory[pc+1];
      *acc = *reg4;
      break;
    case 0xED: /* mov a, r5 */
      addr = memory[pc+1];
      *acc = *reg5;
      break;
    case 0xEE: /* mov a, r6 */
      addr = memory[pc+1];
      *acc = *reg6;
      break;
    case 0xEF: /* mov a, r7 */
      addr = memory[pc+1];
      *acc = *reg7;
      break;
    case 0xF0: /* movx @dptr, a */
      addr = *dpl + *dph*0x100;
      xram[addr] = *acc;
      break;
    case 0xF1: /* acall addr */
      pc += 2;
      push(getLow(pc));
      push(getHigh(pc));

      pc = 0x700 + memory[pc+1];
      return;
      break;
    case 0xF2: /* movx @r0, a */
      xram[*reg0] = *acc;
      break;
    case 0xF3: /* movx @r1, a */
      xram[*reg1] = *acc;
      break;
    case 0xF4: /* cpl a */
      *acc ^= 0xFF;
      break;
    case 0xF5: /* mov addr, a */
      addr = memory[pc+1];
      ram[addr] = *acc;
      break;
    case 0xF6: /* mov @r0, a */
      ram[*reg0] = *acc;
      break;
    case 0xF7: /* mov @r1, a */
      ram[*reg1] = *acc;
      break;
    case 0xF8: /* mov r0, a */
      *reg0 = *acc;
      break;
    case 0xF9: /* mov r1, a */
      *reg1 = *acc;
      break;
    case 0xFA: /* mov r2, a */
      *reg2 = *acc;
      break;
    case 0xFB: /* mov r3, a */
      *reg3 = *acc;
      break;
    case 0xFC: /* mov r4, a */
      *reg4 = *acc;
      break;
    case 0xFD: /* mov r5, a */
      *reg5 = *acc;
      break;
    case 0xFE: /* mov r6, a */
      *reg6 = *acc;
      break;
    case 0xFF: /* mov r7, a */
      *reg7 = *acc;
      break;
    }
  pc += cpu_instr_tkn[memory[pc]][INSTR_TKN_BYTES];
  addr = *psw & (rs1 + rs0);
  if (reg0 - ram != addr)
    {
      reg0 = ram + addr;     reg1 = ram + addr + 1; 
      reg2 = ram + addr + 2; reg3 = ram + addr + 3;
      reg4 = ram + addr + 4; reg5 = ram + addr + 5; 
      reg6 = ram + addr + 6; reg7 = ram + addr + 7;
    }
}

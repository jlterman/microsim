/*************************************************************************************

    Copyright (c) 2003, 2004 by James L. Terman
    This file is part of the 6502 simulator backend

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
#include <assert.h>

#define SIM_CPU_LOCAL

#include "asmdefs.h"
#include "err.h"
#include "asm.h"
#include "front.h"
#include "proc.h"
#include "cpu.h"

/* defintions for P status register */

#define sign 128
#define ov    64
#define brk   16
#define bcd    8
#define intr   4
#define zero   2
#define carry  1

#define STACK_BASE 0x100

#define setP(n, m) setBit(n, &psr, m);
#define setC(n) setBit(n, &psr, carry)
#define getC() (psr & carry)

/* Allocation of internal registers
 */
static int acc, xreg, yreg, psr, sptr;

/* table of register use for each 6502 instruction
 * that is not in the parameter list
 */
static int *instrReg_table[LAST_PROC_TOKEN - PROC_TOKEN] = 
  { 
     NULL,  NULL,  NULL,   NULL,  NULL,  NULL,  NULL,  NULL, 
     NULL,  NULL,  NULL,   NULL,  NULL,  NULL,  NULL,  NULL, 
     NULL,  NULL,  NULL,   NULL,  NULL,  NULL,  &acc, &xreg, 
    &yreg,  NULL, &xreg,  &yreg,  NULL,  NULL, &xreg, &yreg, 
     NULL,  NULL,  &acc,  &xreg, &yreg,  NULL,  NULL,  NULL, 
     NULL,  NULL,  NULL,   &acc,  &psr,  NULL,  NULL,  NULL, 
     NULL,  NULL,  NULL,   NULL,  NULL,  NULL,  NULL,  NULL, 
     &acc, &xreg, &yreg,  &xreg, &yreg,  NULL, &xreg,  NULL, 
    &yreg,  NULL,  NULL
};

/* put value on top of stack and decrement stack pointer by 1 
 */
static void pushStack(int data)
{
  memory[STACK_BASE + sptr] = acc;
  dec(sptr);
}

/* increase stack pointer and return value from top of stack
 */
static int popStack(void)
{
  inc(sptr);  
  return memory[STACK_BASE + sptr];
}

/* Put cpu in correct state after reset
 */
void reset(void)
{
  acc = xreg = yreg = psr = 0;
  sptr = BYTE_MAX - 1;
  pc = memory[0xFFFC] + memory[0xFFFD]*BYTE_MAX;
}

/* add value to acc with the proper setting of status registers.
 * carry is used according to carryFlag but always set afterwards
 */
static void doAdd(int value)
{
  int dlo, dhi;
  if (psr & bcd)
    {
      dlo = (acc & LO_NYBLE) + (value & LO_NYBLE) + getC();
      dhi = (acc & HI_NYBLE) + (value & HI_NYBLE) + dlo/10;
      dlo %= 10;
      setP(dhi>9, carry);
      dhi %= 10;
      acc = dlo + 10*dhi;
    }
  else
    {
      acc += value + getC();
      setP(acc >= (BYTE_MAX-1), carry);
      acc &= BYTE_MASK;
    }
  setP(acc & sign, sign);
  setP(((acc & BIT7_MASK) != 0) ^ ((acc & BIT6_MASK) != 0), ov);
  setP(!acc, zero);
}

/* doSub sub value from acc setting status flags correctly afterwards
 */
static void doSub(int value)
{
  int dlo, dhi;
  if (psr & bcd)
    {
      dlo = (acc & LO_NYBLE) - (value & LO_NYBLE) - !getC();
      dhi = (acc & HI_NYBLE) - (value & HI_NYBLE) - (dlo<0);
      if (dlo<0) dlo += 10;
      setP(dhi >= 0, carry);
      if (dhi<0) dlo += 10;
      acc = dlo + 10*dhi;
    }
  else
    {
      acc -= value + !getC();
      setP(acc >= 0, carry);
      acc &= BYTE_MASK;
    }
  setP(acc & sign, sign);
  setP(((acc & BIT7_MASK) != 0) ^ ((acc & BIT6_MASK) != 0), ov);
  setP(!acc, zero);
}

/* getParam will interpet the parameters of an opcode from
 * the entry in cpu_instr_tkn. It returns a pointer to the 
 * parameter either in memory, or the register
 */
static int *getParam(int op, int *index, int *codeptr)
{
  int t, *addr = memory;

  while ((t = cpu_instr_tkn[op][*index]))
    {
      switch (t)
	{
	case 0:
	case rightPar:
	  return addr;
	  break;
	case pound:
	  ++(*index);
	case rel_addr:
	case data_8:
	  return codeptr; /* return pointer to code for value */
	  break;
	case comma:       /* skip */
	  break;
	case a:
	  return &acc;    /* return pointer to acc */
	  break;
	case x:
	  addr += xreg;   /* x reg param is always added to address */
	  break;
	case y:
	  addr += yreg;   /* y reg param is always added to address */
	  break;
	case addr_16:     /* get 16 bit address from code */
	  addr += *codeptr;          ++codeptr;
	  addr += *codeptr*BYTE_MAX; ++codeptr;
	  break;
	case addr_8:      /* get 8 bit address from code */
	  addr += *(codeptr++);
	  break;
	case leftPar:
	  /* Make recursive call to getParam to evaluate param 
	   * inside ()'s. The address is derefenced to get the
	   * address to the parameter value
	   *
	   * param value is 8-bit except for jmp (addr_16)
	   */
	  addr += *(getParam(op, index, codeptr));
	  if (cpu_instr_tkn[op][INSTR_TKN_INSTR] == jmp)
	    addr += *(getParam(op, index, codeptr) + 1);
	  break;
	default:
	  assert(TRUE);
	  break;
	}
      ++(*index);
    }
  return addr;
}

/* step() is the master function to update the registers and memory 
 * from the execution of the opcode at memory[pc]
 */
void step(void)
{
  int op = memory[pc], index = INSTR_TKN_PARAM, codeptr = pc + 1,
    opcode = cpu_instr_tkn[op][INSTR_TKN_INSTR], n, *param, 
    *reg = instrReg_table[op];

  /* value of pc during instr execution is pc of next instr
   * return immediately if pc has overflowed (let sim register error)
   */  
  pc += cpu_instr_tkn[memory[pc]][INSTR_TKN_BYTES];
  if (pc>MEMORY_MAX) return;

  /* evaluate op parameters and return pointer to its value
   */
  param = getParam(op, &index, &codeptr);
  switch (opcode)
    {
    case adc: /* add memory location with carry to accumulator */
      doAdd(*param);
      break;
    case and: /* and memory location with accumultor */
      acc &= *param;
      setP(acc & sign, sign);
      setP(!acc, zero);
      break;
    case asl: /* arithmatic shift memory location or accumulator left */
      *param *= 2;
      setP(*param >= BYTE_MAX, carry);
      *param &= BYTE_MASK;
      setP(*param & sign, sign);
      setP(!*param, zero);
      break;
    case bcc: /* branch if carry clear */
      if (!getC()) relJmp(pc, *param);
      break;
    case bcs: /* branch if carry set */
      if (getC()) relJmp(pc, *param);
      break;
    case beq: /* branch if zero */
      if (psr & zero) relJmp(pc, *param);
      break;
    case bit: /* bit test memory location */
      setP(*param & acc, zero);
      setP(*param & BIT7_MASK, sign);
      setP(*param & BIT6_MASK, ov);
      break;
    case bmi: /* branch if negative */
      if (psr & sign) relJmp(pc, *param);
      break;
    case bne: /* branch if not equal */
      if (!(psr & zero)) relJmp(pc, *param);
      break;
    case bpl: /* branch if positive */
      if (!(psr & sign)) relJmp(pc, *param);
      break;
    case brk: /* force break */
      setP(1, brk);
      pushStack(getHigh(pc));
      pushStack(getLow(pc));
      pushStack(p);
      setP(1, intr);
      pc = memory[0xFFFE] + memory[0xFFFF]*BYTE_MAX;
      break;
    case bvc: /* branch if overflow clear */
      if (!(psr & ov)) relJmp(pc, *param);
      break;
    case bvs: /* branch if overflow set */
      if (psr & ov) relJmp(pc, *param);
    case clc: /* clear carry */
      setC(0);
      break;
    case cld: /* clear bcd mask */
      setP(0, bcd);
      break;
    case cli: /* clear interrupt mask (enable interrupts) */
      setP(0, intr);
      break;
    case clv: /* clear overflow mask */
      setP(0, bcd);
      break;
    case cmp: /* compare memory and accumulator */ 
    case cpx: /* compare memory and x register */ 
    case cpy: /* compare memory and y register */ 
      n = *reg - *param;
      setP(n & BIT7_MASK, sign);
      setP(!n, zero);
      setP(n >= 0, carry);
      break;
    case dec: /* decrease memory location */
      reg = param;
    case dex: /* decrease x register */
    case dey: /* decrease y register */
      dec(*reg);
      setP(*reg & sign, sign);
      setP(!*reg, zero);
      break;
    case eor: /* exclusive or accumulator and memory */
      acc = acc ^ *param;
      setP(acc & sign, sign);
      setP(!acc, zero);
      break;
    case inc: /* increase memory location */
      reg = param;
    case inx: /* increase x register */
    case iny: /* increase y register */
      inc(*reg);
      setP(*reg & sign, sign);
      setP(!*reg, zero);
      break;
    case jsr: /* subroutine call to address */
      pushStack(getHigh(pc));
      pushStack(getLow(pc));
    case jmp: /* param has address of memory location to be executed next */
      pc = param - memory;
      break;
    case lda: /* load accumulator from memory */
    case ldx: /* load x register  from memory */
    case ldy: /* load y register  from memory */
      *reg = *param;
      setP(*reg & sign, sign);
      setP(!*reg, zero);
      break;
    case lsr: /* logical shift right of accumulator or memory location */
      setC(*param & carry);
      *param /= 2;
      setP(*param & sign, sign);
      setP(!*param, zero);
      break;
    case nop: case NOP: /* no operation */
      break;
    case ora: /* logically or memory with accumulator */
      acc |= *param;
      setP(acc & sign, sign);
      setP(!acc, zero);
      break;
    case pha: /* push accumulator onto stack */
    case php: /* push status reg  onto stack */
      pushStack(*reg);
      break;
    case pla: /* push accumulator from stack */
      acc = popStack();
      setP(acc & sign, sign);
      setP(!acc, zero);
      break;
    case plp: /* push status reg  from stack */
      psr = popStack();
      break;
    case rol: /* rotate memory or accumulator left through carry */
      *param = *param*2 + getC();
      setC(*param>=BYTE_MAX);
      *param &= BYTE_MASK;
      setP(*param & sign, sign);
      setP(!*param, zero);
      break;
    case ror: /* rotate memory or accumulator right through carry */
      *param += 2*getC()*BIT7_MASK;
      setC(*param & carry);
      *param /= 2;
      *param &= BYTE_MASK;
      setP(*param & sign, sign);
      setP(!*param, zero);
      break;
    case rti:
      psr = popStack();
    case rts:
      pc = popStack() + BYTE_MASK*popStack();
      break;
    case sbc: /* subtract memory location with borrow from accumulator */
      doSub(*param);
      break;
    case sec: /* set carry */
      setC(1);
      break;
    case sed: /* set bcd mask */
      setP(1, bcd);
      break;
    case sei: /* set interrupt mask (disable interrupts) */
      setP(1, intr);
      break;
    case sta: /* store accumulator in memory */
    case stx: /* store x register  in memory */
    case sty: /* store y register  in memory */
      *param = *reg;
      break;
    case tax: /* move accumulator into x register */
    case tay: /* move accumulator into y register */
      *reg = acc;
      setP(*reg & sign, sign);
      setP(!*reg, zero);
      break;
    case tsx: /* move stack pointer into x register */
      xreg = sp;
      setP(xreg & sign, sign);
      setP(!xreg, zero);
      break;
    case txs: /* move x register into stack pointer */
      sptr = xreg;
      setP(xreg & sign, sign);
      setP(!xreg, zero);
      break;
    case txa: /* move x register into accumulator */
    case tya: /* move y register into accumulator */
      acc = *reg;
      setP(*reg & sign, sign);
      setP(!*reg, zero);
      break;
    default:
      assert(TRUE);
      break;
    }
}

/* getRegister will return the address to the name of the register given it
 * if *bit not UNDEF, register is one bit in length
 */
int *getRegister(str_storage name, int *bit)
{
  int index;
  const str_storage *ret = bsearch(name, tokens, tokens_length, sizeof(str_storage), &cmpstr);
  if (!ret) return NULL;
  *bit = UNDEF;
  index = ret - tokens + PROC_TOKEN;
  switch (index)
    {
    case pc_reg: return &pc;   break;
    case a:      return &acc;  break;
    case x:      return &xreg; break;
    case y:      return &yreg; break;
    case sp:     return &sptr; break;
    case p:      return &psr;  break;
    default: return NULL;      break;
    }
}

/* getMemory() will return the value of the memor location addr. m is ignored
 */
int *getMemory(int addr, char m)
{
  if (m == '\0' && m != 'c') return NULL;
  if (addr>=MEMORY_MAX) return NULL;
  return memory + addr;
}


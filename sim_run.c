/*************************************************************************************

    Copyright (c) 2003 - 2005 by James L. Terman
    This file is part of the Simulator

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
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>

#ifdef __WIN32__
#include <io.h>
#endif

#define SIM_LOCAL

#include "asmdefs.h"
#include "asm.h"
#include "cpu.h"
#include "err.h"
#include "sim.h"

static brk_struct* brk_table = NULL;
static int num_brk = 1;
static int size_brk = 0;

int run_sim = FALSE;         /* true when simulator is running */

const str_storage simErrMsg[LAST_SIM_ERR - LAST_EXPR_ERR] = 
  {
    "Program counter has overflowed",
    "Bad address parameter"
  };

/* global function called from assembler for memory reference
 * Evalues memory location expression $addr[:c]
 */
int *getMemExpr(char *expr)
{
  int addr, bit;
  char c = '\0';

  if (!run_sim) return NULL;

  if (expr[0] == '$')
    {
      if (expr[strlen(expr) - 2] == ':')
	{
	  c = expr[strlen(expr) - 1];
	  expr[strlen(expr) - 2] = '\0';
	}
      addr = getExpr(expr + 1);
      if (c) expr[strlen(expr) - 2] == ':';
      return getMemory(addr, c);
    }
  else if (expr[0] == '@')
    {
      if (!strcmp(expr + 1, "pc")) return &pc;
      return getRegister(expr + 1, &bit, &addr);
    }
  else
    return NULL;
}

/* first break in array reserved for next command
 */
void setNextBrk(int addr)
{
  safeAddArray(brk_struct, brk_table, num_brk, size_brk);
  brk_table[0].tmp = TRUE;
  brk_table[0].used = TRUE;
  brk_table[0].pc = addr;
  brk_table[0].op = memory[addr];
  brk_table[0].expr = NULL;
}

/* Add a break at the memory address given
 */
int addBrk(int tmpFlag, int addr, char *expr)
{
  safeAddArray(brk_struct, brk_table, num_brk, size_brk);
  brk_table[0].used = FALSE;

  brk_table[num_brk].tmp = tmpFlag;
  brk_table[num_brk].used = TRUE;
  brk_table[num_brk].pc = addr;
  brk_table[num_brk].op = memory[addr];
  brk_table[num_brk].expr = expr;
  return num_brk++;
}

/* print break number. Print all if UNDEF, return TRUE if break exists
 */
int printBreak(FILE* fd, int addr)
{
  int i, brk;
  if (addr == UNDEF)
    {
      for (i = 1; i<num_brk; ++i) printOneBreak(fd, i, brk_table + i);
      return TRUE;
    }

  brk = -memory[pc] - 1;
  if (brk_table[brk].used)
    return printOneBreak(fd, brk, brk_table + brk);
  else
    return FALSE;
}

/* Delete break number brk. If brk == UNDEF, clear all breaks
 */
void delBrk(int brk)
{
  int i;
  if (brk == UNDEF)
    {
      for (i = 1; i<num_brk; ++i) delBrk(i);
      return;
    }
  brk_table[brk].used = FALSE;
  free(brk_table[brk].expr); /* free expr string, if any */
}

/* stepone will execute one instruction. It will step over any break
 */
void stepOne(void)
{
  int oldPC = pc, brk = -memory[pc] - 1;
  if (brk>=0) memory[pc] = brk_table[brk].op;
  step();
  if (pc>=MEMORY_MAX) longjmp(err, bad_pc);
  if (brk>=0) memory[oldPC] = -brk - 1;
}

/* run will start executing at pc or the address given it until it 
 * encounters break. run will return the current pc at break
 */
int run(int addr, int trace)
{
  char *expr;
  int brk, brkFnd;
  if (addr == UNDEF) longjmp(err, bad_addr);
  if (pc >= MEMORY_MAX || pc<0) longjmp(err, bad_pc);

  for (brk = 0; brk < num_brk; ++brk)
    {
      if (brk_table[brk].used)
	{
	  memory[brk_table[brk].pc] = -brk - 1;
	}
    }

  if (memory[pc]<0) stepOne();
  if (trace) traceDisplay();
  while (TRUE)
    {
      brkFnd = -memory[pc] - 1;
      if ((brkFnd>0) && (!(expr = brk_table[brkFnd].expr) || getExpr(expr))) break;
      stepOne();
      if (trace) traceDisplay();
    }

  for (brk = 0; brk < num_brk; ++brk)
    {
      if (brk_table[brk].used)
	{
	  memory[brk_table[brk].pc] = brk_table[brk].op;
	}
    }

  brk_table[0].used = FALSE;
  if (brk_table[brkFnd].tmp) delBrk(brkFnd);
  return brk_table[brkFnd].pc;
}

/* next sets up a temporary break after and jumps over subroutine calls
 * and then calls run
 */
int next()
{
  if (isJSR(memory[pc])) addBrk(TRUE, pc, NULL);
  return run(UNDEF, FALSE);
}


/*************************************************************************************

    Copyright (c) 2003, 2004 by James L. Terman
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

/* break structure entry
 */
typedef struct
{
  int tmp;    /* True if break cleared when hit */
  int used;   /* True if entry a valid break */
  int pc;     /* address of break */
  int op;     /* opcode that break entry in memory[] replaced */
  char *expr; /* if not NULL, break only if expr evaluates non-zero */
} brk_struct;

static brk_struct* brk_table = NULL;
static int num_brk = 0;
static int size_brk = 0;
int run_sim = FALSE;

const str_storage simErrMsg[LAST_SIM_ERR - LAST_EXPR_ERR] = 
  {
    "Program counter has overflowed"
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
	  expr[strlen(expr) - 2] == '\0';
	}
      addr = getExpr(expr + 1);
      if (c) expr[strlen(expr) - 2] == ':';
      return getMemory(addr, c);
    }
  else if (expr[0] == '@')
    {
      if (!strcmp(expr + 1, "pc")) return &pc;
      return getRegister(expr + 1, &bit);
    }
  else
    return NULL;
}

/* Add a break at the memory address given
 */
void addBrk(int tmpFlag, int addr)
{
  if (num_brk >= size_brk) 
    safeRealloc(brk_table, brk_struct, size_brk += CHUNK_SIZE);
  brk_table[num_brk].tmp = tmpFlag;
  brk_table[num_brk].used = TRUE;
  brk_table[num_brk].pc = addr;
  brk_table[num_brk].op = memory[pc];
  brk_table[num_brk].expr = NULL;
  memory[pc] = -num_brk-1;
  ++num_brk;
}

/* print this break. Return true if printable, otherwise false
 */
static int printOneBreak(FILE* fd, int i)
{
  if (!brk_table[i].tmp && brk_table[i].used)
    {
      fprintf(fd, "%2d: Break at $%04X", i, brk_table[i].pc);
      if (brk_table[i].expr) fprintf(fd, " if %s", brk_table[i].expr);
      fprintf(fd, "\n");
      return TRUE;
    }
  else
    return FALSE;
}

/* print break number. Print all if UNDEF, return TRUE if break exists
 */
int printBreak(FILE* fd, int brk)
{
  int i;
  if (brk == UNDEF)
    {
      for (i = 0; i<num_brk; ++i) printOneBreak(fd, i);
      return TRUE;
    }
  else if (brk < num_brk && brk_table[brk].used)
    {
      return printOneBreak(fd, brk);
    }
  else
    return FALSE;
}

/* Delete break number brk. If brk == UNDEF, clear all breaks
 */
void delBrk(int brk)
{
  if (brk == UNDEF)
    {
      num_brk = 0; return;
    }
  memory[brk_table[brk].pc] = brk_table[brk].op;
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
  if (pc>MEMORY_MAX) longjmp(err, bad_pc);
  if (brk>=0) memory[oldPC] = -brk - 1;
}

/* run will start executing at pc or the address given it until it 
 * run will return the current pc
 */
int run(int addr)
{
  int brk;
  if (addr != UNDEF) pc = addr;
  if (memory[pc]<0) stepOne();
  while (memory[pc]>=0) stepOne();
  brk = -memory[pc] - 1;
  if (brk_table[brk].tmp) 
    {
      delBrk(brk);
      return UNDEF;
    }
  else
    return brk_table[brk].pc;
}

/* next sets up a temporary break after and jumps over subroutine calls
 * and then calls run
 */
int next()
{
  if (isJSR(memory[pc])) addBrk(TRUE, pc);
  return run(UNDEF);
}


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

#ifndef _SIM_HEADER
#define _SIM_HEADER
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

enum sim_err 
  {
    bad_pc = LAST_EXPR_ERR, bad_addr, LAST_SIM_ERR
  };

#ifndef SIM_LOCAL
extern const str_storage simErrMsg[];
#endif

#define getSimErrMsg(x) ((x) < LAST_SIM_ERR)

/* global function called from assembler for memory reference
 */
#ifndef SIM_LOCAL
extern
#endif
int *getMemExpr(char*, int*, char*);

/* set a temporary break for next command
 */
#ifndef SIM_LOCAL
extern
#endif
void setNextBrk(int);

/* add break at a line of assembly
 */
#ifndef SIM_LOCAL
extern
#endif
int addBrk(int, int, char*);

/* Delete break number brk
 */
#ifndef SIM_LOCAL
extern
#endif
void delBrk(int);

/* stepone will execute one instruction
 */
#ifndef SIM_LOCAL
extern
#endif
void stepOne(void);

/* run will start executing at pc or the address given it until break
 */
#ifndef SIM_LOCAL
extern
#endif
int run(int, int);

/* print break number. Print all if UNDEF, return TRUE if break exists
 */
#ifndef SIM_LOCAL
extern
#endif
int printBreak(FILE* fd, int brk);

#ifdef SIM_LOCAL
extern
#endif
int printOneBreak(FILE*, int, brk_struct*);

#ifndef SIM_LOCAL
extern int run_sim; /* set true, when simulator is running */
#endif

#ifndef MAIN_LOCAL
extern void traceDisplay();
#endif

#endif

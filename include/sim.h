/*************************************************************************************

    Copyright (c) 2003, 2004 by James L. Terman
    This file is part of the 8051 Simulator

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

enum sim_err 
  {
    bad_pc = LAST_EXPR_ERR, LAST_SIM_ERR
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
int *getMemExpr(char*);

/* add break at a line of assembly
 */
#ifndef SIM_LOCAL
extern
#endif
void addBrk(int tmpFlag, int line);

/* Delete break number brk
 */
#ifndef SIM_LOCAL
extern
#endif
void delBrk(int brk);

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
int run(int);

/* print break number. Print all if UNDEF, return TRUE if break exists
 */
#ifndef SIM_LOCAL
extern
#endif
int printBreak(FILE* fd, int brk);

#ifndef SIM_LOCAL
extern int run_sim; /* set true, when simulator is running */
#endif

#endif

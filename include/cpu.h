/*************************************************************************************

    Copyright (c) 2003, 2004 by James L. Terman
    This file is part of the Assembler

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
#ifndef _CPU_HEADER
#define _CPU_HEADER

#include "asmdefs.h"
#include "err.h"
#include "asm.h"
#include "front.h"

/* The following file defines the processor specific functions that must be
 * defined in the processor specific directory. CPU_LOCAL functions are 
 * defined in asm.c and SIM_CPU_LOCAL functions are defined in sim.c
 */

#ifndef PROC_LOCAL
extern
#endif
void storeWord(int*, int);

#ifndef CPU_LOCAL
extern 
#endif
int isInstrOp(int);

#ifndef CPU_LOCAL
extern 
#endif
int handleInstr(const int*, int*);

#ifndef CPU_LOCAL
extern 
#endif
int handleTkn(const int, int*, int*);


#define isConstToken(t) (((t)>CONST_TOKEN && (t)<PROC_TOKEN) || (isConstProcTkn(t)))

#ifndef CPU_LOCAL
extern 
#endif
int isConstProcTkn(int);

#ifndef CPU_LOCAL
extern 
#endif
int isJSR(int);

/*
 *
 *  Processor specific simulator Definitions
 *
 */
#ifndef SIM_CPU_LOCAL
extern 
#endif
void reset(void);

#ifndef SIM_CPU_LOCAL
extern 
#endif
int irq(int);

#ifndef SIM_CPU_LOCAL
extern 
#endif
void step(void);

#ifndef SIM_CPU_LOCAL
extern 
#endif
int *getRegister(str_storage, int*, int*);

#ifndef SIM_CPU_LOCAL
extern 
#endif
int *getMemory(int, char);

#ifndef SIM_CPU_LOCAL
extern 
#endif
void dumpMemory(FILE*);

/* number of tokens needed to define cpu instr
 */
#define INSTR_TKN_BUF 12

#define INSTR_TKN_OP 0
#define INSTR_TKN_BYTES 1
#define INSTR_TKN_CYCLES 2
#define INSTR_TKN_INSTR 3
#define INSTR_TKN_PARAM 4

#ifndef BACKEND_LOCAL
int memory[MEMORY_MAX]; /* 64K of program memory                   */
int pc;                 /* location of next instr to be assembled  */
#endif

/* the following global variables have to be defined in asm.c:
 */
#ifndef CPU_LOCAL
extern const int cpu_instr_tkn[][INSTR_TKN_BUF]; /* array of cpu instr */
extern const int isCharToken_table[];     /* single char tokens       */
extern const int isToken_table[];         /* legal char tokens        */
extern const str_storage tokens[];        /* list of legal tokens     */
extern const int tokens_length;           /* no. of legal tokens      */
extern const int isRegister_table[];      /* table of register tokens */

extern char cpu_version[];            /* expected by main.c and sim.c */
extern const label_type def_labels[]; /* expected by expr.c           */
#endif

#define relJmp(pc, rel) (pc += (rel) - (((rel)>SGN_BYTE_MAX) ? BYTE_MAX : 0))
#define inc(x) (((x)==(BYTE_MAX-1)) ? (x) = 0 : ++(x))
#define dec(x) ((x) = (!(x)) ? BYTE_MAX-1 : (x) - 1)
#define setBit(x, addr, bit) (*(addr) = (x) ? *(addr) | bit : *(addr) & (BYTE_MASK - (bit)))

#endif

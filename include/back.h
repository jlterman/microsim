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

/* number of tokens needed to define cpu instr
 */
#define INSTR_TKN_BUF 16

#define INSTR_TKN_OP 0
#define INSTR_TKN_BYTES 1
#define INSTR_TKN_CYCLES 2
#define INSTR_TKN_INSTR 3

#ifndef BACKEND_LOCAL
extern
#endif
const int *findInstr(const int*);

#ifndef BACKEND_LOCAL
extern
#endif
void loadMemory(const int*, int*);

#ifndef BACKEND_LOCAL
extern
#endif
const char *getToken(int);

#ifndef BACKEND_LOCAL
extern
#endif
void initLabels(void);

#ifndef BACKEND_LOCAL
extern
#endif
void writeList(int, int, char*);


#ifndef CPU_LOCAL
extern const int isCharToken_table[];     /* single char tokens defn'd by proc.h */
extern const int isToken_table[];         /* legeal char tokens defn'd by proc.h */

extern const str_storage tokens[];        /* list of legal tokens defn'd by proc.h */

extern const int isInstr_table[];                /* array of real vs. psuedo instructions */
extern const int cpu_instr_tkn[][INSTR_TKN_BUF]; /* array of cpu instructions */
extern const label_type def_labels[];            /* array of predefined labels for the cpu */
#endif

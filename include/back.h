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
#ifndef _BACK_HEADER
#define _BACK_HEADER

#include "asmdefs.h"

/* the following functions are the global functions provided by back.c
 * for the front end of the assembler
 */
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
void writeList(FILE*, int, int, str_storage);

#endif

/*************************************************************************************

    Copyright (c) 2003 - 2005 by James L. Terman
    This file is part of the Assembler and Simulator

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
#include <setjmp.h>

#define PROC_LOCAL

#include "asmdefs.h"
#include "err.h"
#include "front.h"
#include "cpu.h"
#include "proc.h"

/* this file is for functions whose behavior can change 
 * dependent on the processor
 */

/* Store a 16 bit word depedent on the endian of the processor
 */
void storeWord(int *addr, int value)
{
#if CPU_BIG_ENDIAN
  *addr = getHigh(value); ++addr;
  *addr =  getLow(value);
#else
  *addr =  getLow(value); ++addr;
  *addr = getHigh(value);
#endif
}

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
#ifndef _FRONT_HEADER
#define _FRONT_HEADER

#include "err.h"

/* emum directv are tokens for assmembler psuedo directives 
 * handled by the front end. Note comma is defined here for db & dw.
 */
enum dirctv
  {
    db = 1, dw, equ, org, DIRCTV_END
  };

/* enum const_tokens are tokens that represent different types of 
 * constants that the opcodes takes as parameters
 */
enum const_tokens 
  {
    comma = DIRCTV_END, CONST_TOKEN, character,
    data_1, data_2, data_3, data_4, data_5, data_6, data_7, data_8,
    addr_1, addr_2, addr_3, addr_4, addr_5, addr_6, addr_7, addr_8,
    CONST_WORD, rel_addr, number, expr, label, addr_label, tmpaddr_label,
    data_9, data_10, data_11, data_12, data_13, data_14, data_15, data_16,
    addr_9, addr_10, addr_11, addr_12, addr_13, addr_14, addr_15, addr_16,
    PROC_TOKEN
  };

extern FILE *lst; /* File descriptor of assembly list file */
#endif

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
#ifndef _FRONTEND
#define _FRONTEND

/* emum directv are tokens for assmembler psuedo directives 
 * handled by the front end. Note comma is defined here for db & dw.
 */
#define DIRCTV_BASE 1024

enum dirctv
  {
    db = DIRCTV_BASE, dw, equ, org
  };

/* enum const_tokens are tokens that represent different types of 
 * constants that the opcodes takes as parameters
 */
enum const_tokens 
  {
    comma = 511, const_tok, data_8, data_16, addr_8, addr_16, rel_addr,
    number, character, expr, label, addr_label
  };

/* Undefined label value using guarenteed illegal value
 */
#define UNDEF -MEMORY_MAX

#define SRCH_BIT_MAX 15

#define LBL_BUF 1024
#define LBL_MAX 2<<(SRCH_BIT_MAX + 1)

#endif

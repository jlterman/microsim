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
    comma = DIRCTV_END, CONST_TOKEN, 
    data_8, data_16, addr_8, addr_16, rel_addr,
    number, character, expr, label, addr_label, PROC_TOKEN
  };

enum asm_err 
  {
    /* front end errors */

    no_char = LAST_EXPR_ERR, undef_org, noexpr_org, miss_colon, 
    bad_equ, bad_db, bad_char,

    /* Back End Error messages */

    bad_instr, no_instr, const_range, addr8_range, rel_range,
    LAST_ASM_ERR
  };

extern FILE *lst; /* File descriptor of assembly list file */
#endif

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
#ifndef _ERR_HEADER
#define _ERR_HEADER

#include <setjmp.h>

#ifndef EXPR_LOCAL
extern
#endif
jmp_buf err;                    /* used for error handling */

/* expression error codes. These error may happen anywhere and are global
 */
enum expr_err
  {
    undef_label, labval_undef, bad_number, bad_expr, zero_div, 
    no_op, miss_par, no_leftPar, no_eq, no_mem, bad_reg, LAST_EXPR_ERR
  };

#ifndef EXPR_LOCAL
extern const str_storage exprErrMsg[];
#endif

#define getExprErrMsg(x) (((x) < LAST_EXPR_ERR && (x) >= 0) ? exprErrMsg[x] : NULL)

#endif

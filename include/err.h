/*************************************************************************************

    Copyright (c) 2003, 2004 by James L. Terman
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
#ifndef _ERR_HEADER
#define _ERR_HEADER

#include <setjmp.h>
#include "asmdefs.h"

#ifndef EXPR_LOCAL
extern
#endif
jmp_buf err;                    /* used for error handling */


/* expression error codes.
 */
enum expr_err
  {
    no_error, undef_label, labval_undef, bad_number, bad_expr, zero_div, 
    no_op, miss_par, no_leftPar, no_eq, no_mem, bad_reg, LAST_EXPR_ERR
  };

/* assembler error codes
 */
enum asm_err 
  {
    /* front end errors */

    no_char = LAST_EXPR_ERR, undef_org, noexpr_org, miss_colon, 
    bad_equ, bad_db, bad_char, bad_addr, bad_tmplbl, undef_tmplbl,
    illegal_equ,

    /* Back End Error messages */

    dup_instr, bad_instr, no_instr, rel_range, 
    data1_range, data2_range, data3_range, data4_range, 
    data5_range, data6_range, data7_range, data8_range,
    data9_range,  data10_range, data11_range, data12_range,
    data13_range, data14_range, data15_range, data16_range,
    addr1_range, addr2_range, addr3_range, addr4_range, 
    addr5_range, addr6_range, addr7_range, addr8_range,
    addr9_range,  addr10_range, addr11_range, addr12_range,
    addr13_range, addr14_range, addr15_range, addr16_range,
    LAST_ASM_ERR
  };

/* list of simulator command line errors
 */
enum cmd_err
  { 
    miss_param = LAST_ASM_ERR, extra_param, out_range, bad_base, 
    bad_op, no_brk, no_reg, no_dsp, dup_brk, no_irq, sim_addr, bad_param,
    run_intr, no_cmd, LAST_CMD_ERR
  };

/* list of generic processor execution errors
 */
enum step_err 
  {
    pc_overflow = LAST_CMD_ERR, stack_overflow, stack_underflow, 
    divide_zero, nonexst_op, LAST_ERR
  };

#ifndef MAIN_LOCAL
extern const str_storage error_messages[];
#endif

#ifndef SIM_CPU_LOCAL
extern const str_storage proc_error_messages[];
#endif

#endif

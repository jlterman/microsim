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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#define BACKEND_LOCAL

#include "asmdefs.h"
#include "err.h"
#include "front.h"
#include "back.h"
#include "cpu.h"

int memory[MEMORY_MAX] = { 0 };   /* 64K of program memory                   */
int pc = 0;                       /* location of next instr to be assembled  */

/* writeListLine will a print a line of the assembly listing consisting
 * of the line number, address, opcodes, and the original line of assembly.
 * If opcodes more than 3, only the address and opcodes are pritned.
 */
static void writeListLine(FILE *lst, int oldPC, int line, str_storage buffer)
{
  int p;

  if (line) 
    fprintf(lst, "%5d %04X: ", line, oldPC);
  else
    fprintf(lst, "      %04X: ", oldPC);

  for (p=0; p<3; ++p) 
    {
      if (p < (pc - oldPC))
	{
	  fprintf(lst, "%02X ", memory[oldPC+p]);
	}
      else /* no more bytes to print, print space instead */
	{
	  fprintf(lst, "   ");
	}
    }
  fprintf(lst, "%s\n", buffer);
}

/* writeList() is called to output a line of assembly listing
 * to the file descrp lst after each line is processed by
 * second pass.
 */
void writeList(FILE *lst, int oldPC, int line, str_storage buffer)
{
  while (pc>=oldPC)
    {
      writeListLine(lst, oldPC, line, buffer);

      /* if line generates more than 3 bytes (db, dw, str or asc),
       * print these bytes but with blank line
       */
      oldPC += 3; buffer = ""; line = 0;
    }
}

/* matchTokens() looks for the instruction in instr[][] that matches the 
 * list of tokens given to it by the assembly line. A number or expr token
 * will match any constant token (addr_8, addr_16, data_8, etc). Range 
 * checking is done further down-stream
 */
static const int *matchTokens(const int instr[][INSTR_TKN_BUF], const int *tkn)
{
  int i, done, t, p;
  i = 0;
  while (instr[i][0]>=0)
    {
      done = 1;
      t = 0;
      p = INSTR_TKN_INSTR;
      while (instr[i][p])
	{
	  if (isConstToken(tkn[t]))
	    {
	      done &= isConstToken(instr[i][p]);
	      ++t;
	    }
	  else
	    {
	      done &= (tkn[t] == instr[i][p]);
	    }
	  ++p; ++t;
	  if (!done) break;
	}
      if (done && !tkn[t] && !instr[i][p]) return instr[i];
      ++i;
    }
  return NULL;
}

/* findInstr() looks for the instr that matches the tokens given it
 */
const int *findInstr(const int *tkn)
{
  const int *ret = NULL;

  if (!isInstrOp(tkn[0])) longjmp(err, bad_instr);

  if ( (ret = matchTokens(cpu_instr_tkn, tkn)) )
    return ret;
  else
    longjmp(err, no_instr);
}

/* loadMemory() takes the list of tokens from the line of assmebly and
 * the instruction tokens they match and assembles the line into opcodes
 * in memory. Range checking is done here
 */
void loadMemory(const int *theInstr, int *tkn)
{
  int t, tkn_pos = 0;

  /* Any instruction unique to a paticular processor are handled by 
   * handle instr
   */
  if (handleInstr(theInstr, tkn)) return;

  /* undefined operator means psuedo op such as db, dw, asc or str
   */
  if (theInstr[INSTR_TKN_OP] != UNDEF) memory[pc++] = theInstr[INSTR_TKN_OP];
  for (t=INSTR_TKN_INSTR+1; theInstr[t]; ++t)
    {
      /* handleTkn() handles processor specific tokens
       */
      if (handleTkn(theInstr[t], tkn, &tkn_pos)) continue;
      switch (theInstr[t])
	{
	case data_8:
	  tkn_pos += 2;
	  if (tkn[tkn_pos]>=BYTE_MAX || 
	      tkn[tkn_pos]<SGN_BYTE_MIN) longjmp(err, const_range);
	  memory[pc++] = getLow(tkn[tkn_pos]);
	  break;
	case addr_8:
	  tkn_pos += 2;
	  if (tkn[tkn_pos]>=BYTE_MAX || 
	      tkn[tkn_pos+2]<0) longjmp(err, addr8_range);
	  memory[pc++] = tkn[tkn_pos];
	  break;
	case data_16:
	  if (tkn[tkn_pos+2]>((1<<16) - 1) || 
	      tkn[tkn_pos+2]<-(1<<15)) longjmp(err, const_range);
	case addr_16:
	  tkn_pos += 2;
	  storeWord(memory + pc, tkn[tkn_pos]);
	  pc += 2;
	  break;
	case rel_addr:
	  tkn_pos += 2;
	  if ((tkn[tkn_pos] - pc - 1)>SGN_BYTE_MAX || 
	      (tkn[tkn_pos] - pc - 1)<SGN_BYTE_MIN) longjmp(err, rel_range);
	  memory[pc] = getLow(tkn[tkn_pos] - pc - 1);
	  ++pc;
	  break;
	default:
	  ++tkn_pos;
	  break;
	}
    }
}

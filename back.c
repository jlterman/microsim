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

#define BACKEND_LOCAL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "asm.h"
#include "front.h"
#include "back.h"
#include "proc.h"
#include "err.h"

/* errMsg are the strings for the error values in errNo defined below
 * Note that errNo starts at 257.
 */
enum errNo 
  {
    bad_instr = BACKERR+1, no_instr, const_range, addr8_range, bit_range, rel_range
  };

const str_storage backErrMsg[] = 
  {
    "Unrecognized instruction",
    "No such instruction",
    "8-bit constant out of range", 
    "8-bit address out of range",
    "Illegal bit address",
    "Relative jmp out of range"
  };

void writeList(int oldPC, int line, char* buffer)
{
  int p;
  if (pc>oldPC)
    {
      fprintf(lst, "%5d %04X: ", line, oldPC);
      for (p=0; p<3; ++p) 
	{
	  if (p < (pc - oldPC))
	    {
	      fprintf(lst, "%02X ", memory[oldPC+p]);
	    }
	  else
	    {
	      fprintf(lst, "   ");
	    }
	}
      fprintf(lst, "%s\n", buffer);
    }
  else
    fprintf(lst, "%5d %04X: ""   ""   ""   ""%s\n", line, oldPC, buffer);
}

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
	  if (tkn[t]>const_tok)
	    {
	      done &= (instr[i][p]>const_tok);
	      ++t;
	    }
	  else
	    {
	      done &= (tkn[t] == instr[i][p]);
	    }
	  ++p; ++t;
	  if (!done) break;
	}
      if (done) return instr[i];
      ++i;
    }
  return NULL;
}

const int *findInstr(const int *tkn)
{
  const int *ret = NULL;

  if (!isInstr(tkn[0])) longjmp(err, bad_instr);

  if ( (ret = matchTokens(cpu_instr_tkn, tkn)) )
    return ret;
  else
    longjmp(err, no_instr);
}

void loadMemory(const int *theInstr, int *tkn)
{
  int t, tkn_pos = 0;

  if (handleInstr(theInstr, tkn)) return;

  if (theInstr[INSTR_TKN_OP]<256) memory[pc++] = theInstr[INSTR_TKN_OP];
  for (t=INSTR_TKN_INSTR+1; theInstr[t]; ++t)
    {
      switch (theInstr[t])
	{
	case data_8:
	  tkn_pos += 2;
	  if (tkn[tkn_pos]>BYTE_MAX || 
	      tkn[tkn_pos]<SGN_BYTE_MIN) longjmp(err, const_range);
	  memory[pc++] = getLow(tkn[tkn_pos]);
	  break;
	case addr_8:
	  tkn_pos += 2;
	  if (tkn[tkn_pos]>BYTE_MAX || tkn[tkn_pos+2]<0) longjmp(err, addr8_range);
	  memory[pc++] = tkn[tkn_pos];
	  break;
	case data_16:
	  if (tkn[tkn_pos+2]>((1<<16) - 1) || 
	      tkn[tkn_pos+2]<-(1<<15)) longjmp(err, const_range);
	case addr_16:
	  tkn_pos += 2;

#if ENDIAN == BIG
	  memory[pc++] = getHigh(tkn[tkn_pos]);
	  memory[pc++] = getLow(tkn[tkn_pos]);
#else
	  memory[pc++] = getLow(tkn[tkn_pos]);
	  memory[pc++] = getHigh(tkn[tkn_pos]);
#endif
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

const char *getToken(int i)
{
  if (i>=0 && i<TOKEN_LENGTH) return tokens[i]; else return NULL;
}

void initLabels(void)
{
  int i;

  if (labels) return;
  safeMalloc(labels, label_type, LBL_BUF);
  safeMalloc(sort_labels, int, LBL_BUF);
  
  i = 0;
  while (def_labels[i].value != UNDEF)
    {
      labels[i].name[LBL_LEN_MAX] = '\0';
      strncpy(labels[i].name, def_labels[i].name, LBL_LEN_MAX);
      labels[i].value = def_labels[i].value;
      sort_labels[i] = i;
      ++i;
    }
  num_labels = i;
  size_buf_labels = LBL_BUF;
}

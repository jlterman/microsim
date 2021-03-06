/*************************************************************************************

    Copyright (c) 2003 - 2005 by James L. Terman
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
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <memory.h>
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>

/* This file serves as the interface of the assembler module.
 * Assembler module will call getBuffer() to get next line.
 * List file will be output to lst file descriptor and
 * object file will be output to obj file descriptor.
 *
 * call dopass(firstPass) and dopass(secondPass) to assemble file
 */

#define FRONTEND_LOCAL

#include "asmdefs.h"
#include "err.h"
#include "asm.h"
#include "front.h"
#include "back.h"
#include "cpu.h"

/* table of characters that have a special meaning
 * when slashed for character constants.
 */
#define slashChar(x) slashChar_table[(int) x]

static const char slashChar_table[ASCII_MAX] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '\'',
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '\?', 
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, '\a', '\b', 0, 0, '\f', 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, '\r', 0, '\t', 0, '\v', 0,
    0, 0, 0, 0, 0, 0, 0, 0,
  };

/* Macros that return legal tokens from special to the processor
 * isCharToken_table defined in back.c in processor directory
 */
#define isCharToken(x) isCharToken_table[(int) x]

/* Macros that return legal tokens from special to the processor
 * isToken_table defined in back.c in processor directory
 */
#define isToken(x) (isToken_table[(int) x] | isLabel_table[(int) x])

#define TKN_BUF 256

/* assembly front end tokens
 */
static const int   asm_dirctv_length = DIRCTV_END - 1;
static const str_storage asm_dirctv[DIRCTV_END - 1] = 
  {
    "db", "dw", "equ", "org"
  };

static int oldPC = 0;             /* keeps track of last pc of prev instr    */
static int pos;                   /* used by nextToken() for pos in buffer   */
static char *token = NULL;        /* used by nextToken() to store next token */

/* nextToken reads buffer staring at position pos. The token
 * found is placed in character array token.
 * First it looks for comma, character constant, single char token,
 * token, or label. If none matches it returns possible expr
 * return value of FALSE represents unmatched token
 */
static int nextToken(str_storage buffer)
{
  const str_storage *ret;
  int d, index, end;
  int start = pos; /* start where last call left off */

  while (isspace(buffer[start])) ++start;

  if (!buffer[start]) return FALSE;

   if (buffer[start] == ',') 
     { 
       pos = start + 1; return comma;
     }
   if (buffer[start] == '\'') /* look for char constant */
     {
       index = 0;
       token[index++] = buffer[start++];
       if ( (token[index++] = buffer[start++]) == '\\')
	 token[index++] = buffer[start++];
       if (buffer[start] != '\'') longjmp(err, bad_char);
       token[index++] = buffer[start++];
       token[index] = '\0';
       pos = start;
       return character;
     }

  /* If char is not token or not part of one, or part of expr, return error */

  if (!isCharToken(buffer[start]) && !(isToken(buffer[start]) || isExpr(buffer[start]))) longjmp(err, no_char);

  /* look for single char tokens before all others
   */
  if (isCharToken(buffer[start]))
    {
      pos = start + 1;
      token[0] = buffer[start]; token[1] = '\0';
      
      ret = bsearch(token, tokens, tokens_length, sizeof(str_storage), &cmpstr);
      if (!ret) return FALSE; else return ret - tokens + PROC_TOKEN;
    }
  
  if (!isdigit(buffer[start]) && isToken(buffer[start]))
    { 
      /* possible token, addr label, or label
       */
      end = start + 1;
      while (buffer[end] && (isToken(buffer[end]))) ++end;
      strncpy(token, buffer + start, end - start); token[end - start] = '\0';

      /* if we find a token or a label there may be a space or colon char
       * we need to skip over
       */
      pos=end + (buffer[end]==' ' || buffer[end]==':');
      
      ret = bsearch(token, tokens, tokens_length, sizeof(str_storage), &cmpstr);
      if (ret)
	{
	  return ret - tokens + PROC_TOKEN;        /* found match */
	}
      
      ret = bsearch(token, asm_dirctv, asm_dirctv_length, sizeof(str_storage), &cmpstr);
      if (ret)
	{
	  return ret - asm_dirctv + 1;
	}
      
      if (token[0] == '@') /* look for temp label @n */
	{
	  if (isdigit(token[1]))
	    return (buffer[end] == ':') ? tmpaddr_label : label;
	  else
	    longjmp(err, bad_tmplbl);
	}

      if (isalpha(token[0]))     /* possible label */
	{
	  index = 0;
	  while (token[index])   /* check to see if token is legal label */
	    {
	      if (!isLabel(token[index])) break;
	      ++index;
	    }
	  if (!token[index])        /* if at end of string, legal label */
	    {
	      if (buffer[end]==':') /* if followed by colon, addr label */
		{
		  return addr_label;
		}
	      else if (!buffer[end])
		{
		  return label;
		}
	      else if (buffer[end]) /* label not followed by arith op */
		{
		  while (buffer[end] && isspace(buffer[end])) ++end;
		  if (!isOp(buffer[end])) return label;
		}
	    }
	}
    }

  /* find expression substring. Add to substring as longs as 
   * legal expression char or embedded space
   */
  end = start;
  while (buffer[end] && (isExpr(buffer[end]) || isspace(buffer[end])))
    {
      if (buffer[end] == '(') 
	end = findClosePar(buffer, end) + 1;
      else if (isCharToken(buffer[end]))
	break;
      else
	++end;
    }
  
  d = 0;
  for (index=start; index<end; ++index)
    { 
      /* put expression substr in token, remove all spaces
       */
      if (!isspace(buffer[index])) token[d++] = buffer[index];
    }
  token[d] = '\0';
  pos = end;

  if (isdigit(token[0]))
    {
      index = 0;
      while (token[index])
	/* check to see if token could be simply a number
	 */
	{
	  if (!isalnum(token[index])) break;
	  ++index;
	}
      if (!token[index]) return number; /* may be number, check syntax later */
    }
  return expr;  /* has to be expr, will check syntax later */
}

/* newDataTkn synthesizes a virtual token list for a db or dw psuedo-op
 */
static void newDataTkn(int *dataTkn, int *tkn, int data_token)
{
  int dt = 0, t = 0;

  dataTkn[dt++] = UNDEF; /* undef indicates no opcode */
  dataTkn[dt++] = 0;
  dataTkn[dt++] = 0;
  dataTkn[dt++] = 0;     /* skip over param of non-existant opcode */

  /* number should follow db or dw
   */
  if (tkn[++t] != number) longjmp(err, bad_db);
  ++t;
  dataTkn[dt++] = data_token; /* put const token for number in tkn */
  
  /* legal db or dw will be db(dw) number[[, number], number ...]
   * for every comma, number token in tkn, put a comma, const token in
   * dataTkn
   */
  while (tkn[++t])
    {
      if (tkn[t++] != comma) longjmp(err, bad_db);
      dataTkn[dt++] = comma;
      
      if (tkn[t++] != number) longjmp(err, bad_db);
      dataTkn[dt++] = data_token;
    }
  dataTkn[dt] = 0;
}

/* first pass does not generate any code, but it will check the syntax
 * of the assembly file and it will define all equ labels and all 
 * address labels (pc will be tracked for this purpose)
 */
void firstPass(str_storage buffer)
{
  const int *theInstr;
  int tkn_pos = 0, tkn[TKN_BUF] = { 0 }; /* tokenized buffer */
  int tkn_end, errNo;
  jmp_buf exprErr;
  label_type *l;

  while (tkn_pos<TKN_BUF - 1 && (tkn[tkn_pos] = nextToken(buffer)))
    {
      switch (tkn[tkn_pos])
	{
	case number:
	  tkn[++tkn_pos] = getNumber(token); 
	  break;
	case character:
	  tkn[tkn_pos++] = number;
	  tkn[tkn_pos] = (token[1] == '\\') ? slashChar(token[2]) : token[1];
	  if (!tkn[tkn_pos]) longjmp(err, bad_char);
	  break;
	case expr:
	  setJmpBuf(&exprErr);
	  if ((errNo = setjmp(exprErr)) == 0)
	    {
	      /* Catch any errors from getExpr(token).
	       * If error, leave expr token for undefined value
	       */
	      tkn[tkn_pos + 1] = getExpr(token);
	      tkn[tkn_pos++] = number;
	    }
	  else
	    tkn[++tkn_pos] = UNDEF;
	  restoreJmpBuf();
	  break;
	case label:
	  tkn[++tkn_pos] = setLabel(token, UNDEF, FALSE);
	  break;
	case addr_label: 
	  if ((l = getLabel(token)) && l->value != UNDEF) 
	    longjmp(err, bad_addr);
	case tmpaddr_label:
	  tkn[++tkn_pos] = setLabel(token, pc, FALSE); 
	  break;
	}
      ++tkn_pos;
    }
  if (!tkn_pos) return;
  
  tkn[tkn_pos] = 0; tkn_end = tkn_pos;
  tkn_pos = 2*((tkn[0] == addr_label) || (tkn[0] == tmpaddr_label));
  switch (tkn[tkn_pos])
    {
    case org:
      switch (tkn[tkn_pos + 1])
	{
	case number: pc = tkn[tkn_pos + 2]; break;
	case label:  pc = labels[tkn[tkn_pos + 1]].value; break;
	default:     longjmp(err, noexpr_org); break;
	}
      if (pc<0) longjmp(err, undef_org);
      if (tkn_pos && tkn[tkn_pos - 2] == addr_label)
	labels[tkn[tkn_pos - 1]].value = pc;
      break;
    case label:
      if (tkn[tkn_pos + 2] != equ) longjmp(err, miss_colon);
      if (labels[tkn[tkn_pos + 1]].value != UNDEF) longjmp(err, illegal_equ);
      switch (tkn[tkn_pos + 3])
	{
	case number: labels[tkn[tkn_pos + 1]].value = tkn[tkn_pos + 4]; break;
	case label:  labels[tkn[tkn_pos + 1]].value = labels[tkn[tkn_pos + 4]].value; break;
	case expr:   /* process expr in second pass */ break;
	default:     longjmp(err, bad_expr); break;
	}
      break;
    case db:
      pc += tkn_end/2;
      break;
    case dw:
      pc += tkn_end - 1;
      break;
    default:
      theInstr = findInstr(tkn+tkn_pos);
      pc += theInstr[INSTR_TKN_BYTES]; /* track value of pc for addr labels */
      break;
    }
}

/* second pass will now generate code. All labels have been defined in 
 * first pass. Syntax errors will be reported again and range errors will
 * now also be reported
 */
void secondPass(str_storage buffer)
{
  int tkn_pos = 0;
  int tkn[TKN_BUF] = { 0 };       /* tokenized buffer */
  int dataTkn[TKN_BUF] = { 0 };

  while (tkn_pos<TKN_BUF - 1 && (tkn[tkn_pos] = nextToken(buffer)))
    {
      switch (tkn[tkn_pos])
	{
	case number:
	  tkn[++tkn_pos] = getNumber(token); 
	  break;
	case character:
	  tkn[tkn_pos++] = number;
	  tkn[tkn_pos] = (token[1] == '\\') ? slashChar(token[2]) : token[1];
	  if (!tkn[tkn_pos]) longjmp(err, bad_char);
	  break;
	case expr:
	  tkn[tkn_pos] = number;
	  tkn[++tkn_pos] = getExpr(token);
	  break;
	case label: 
	  if (tkn_pos)
	    {
	      tkn[tkn_pos] = number;
	      tkn[++tkn_pos] = getLabelValue(token);
	    }
	  else
	    {
	      tkn[++tkn_pos] = setLabel(token, UNDEF, FALSE);
	    }
	  break;
	case tmpaddr_label:
	  setLabel(token, pc, TRUE); /* if tmp label, update value */
	case addr_label:
	  --tkn_pos;          /* ignore address labels in 2nd pass */
	  break;
	case equ:
	  if (!nextToken(buffer)) longjmp(err, bad_equ);
	  tkn[++tkn_pos] = number;
	  tkn[++tkn_pos] = getExpr(token);
	  break;
	}
      ++tkn_pos;
    }
  if (!tkn_pos) return;

  tkn[tkn_pos] = 0;
  switch (tkn[0])
    {
    case org:
      writeObj(pc, tkn[2]);
      pc = tkn[2]; oldPC = pc;
      break;
    case label:
      if (tkn[2] != equ || tkn[3] != number || tkn[5] || tkn[1] < 10) longjmp(err, bad_equ);
      labels[tkn[1]].value =  tkn[4];
      break;
    case db:
      newDataTkn(dataTkn, tkn, data_8);
      loadMemory(dataTkn, tkn);
      break;
    case dw:
      newDataTkn(dataTkn, tkn, data_16);
      loadMemory(dataTkn, tkn);
      break;
    default:
      loadMemory(findInstr(tkn), tkn);
      break;
    }
}

/* doPass is main entry into assembler. It will call getBuffer to get 
 * next line of assembly file, strip off leading and lagging white space
 * and strips out extra white space between tokens before passing line
 * on to the pass function given it.
 */
int doPass(passFunc pass)
{
  static char *work_buf = NULL;
  static int  size_work_buf = 0;
  str_storage buffer;
  int errNo, s, d, line = 0, numErr = 0;

  if (!work_buf) safeMalloc(work_buf, char, size_work_buf = CHUNK_SIZE);
  if (!token)    safeMalloc(token, char, size_work_buf)
  if (!labels) initLabels();

  pc = 0; oldPC = 0;
  while ( (buffer = getBuffer()) )
    {
      if (size_work_buf<(strlen(buffer) + 1))
	{
	  size_work_buf = strlen(buffer) + 1;
	  safeRealloc(work_buf, char, size_work_buf);
	  safeRealloc(token, char, size_work_buf);
	}
      ++line;
      s = 0; d = 0;
      while (isspace(buffer[s])) ++s;
      while (buffer[s] && buffer[s] != ';')
	{
	  if (d && isToken(work_buf[d - 1]) && buffer[s] && isToken(buffer[s])) work_buf[d++] = ' ';	  
	  while (buffer[s] && !isspace(buffer[s])) work_buf[d++] = tolower(buffer[s++]);
	  while (isspace(buffer[s])) ++s;
	}
      work_buf[d] = '\0';
      if (d)
	{
	  pos = 0;
	  if ((errNo = setjmp(err)) != 0)
	    {
	      ++numErr;
	      assert(errNo >= 0);
	      printErr(errNo, line);
	      continue;
	    }
	  pass(work_buf);
	}
      if (lst) writeList(lst, oldPC, line, buffer); oldPC = pc;
    }
  return numErr;
}


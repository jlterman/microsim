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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <memory.h>
#include <stdlib.h>
#include <setjmp.h>

/* This file serves as the interface of the assembler module.
 * Assembler module will call getBuffer() to get next line.
 * List file will be output to lst file descriptor and
 * object file will be output to obj file descriptor.
 *
 * call dopass(firstPass) and dopass(secondPass) to assemble file
 */

#define TKN_BUF 256

#define FRONTEND_LOCAL
#include "asm.h"
#include "front.h"
#include "back.h"
#include "err.h"

/* errMsg are the strings for the error values in errNo defined below
 * Note that errNo starts at 1.
 */
enum errNo 
  {
    no_char = FRONTERR+1, undef_label, labval_undef, bad_number, bad_expr, miss_expr, zero_div, 
    no_op, undef_org, noexpr_org, miss_colon, bad_equ, bad_db, bad_char
  };

const str_storage frontErrMsg[] = 
  {
    "Illegal character encountered", 
    "Undefined label",
    "Label value undefined",
    "Unrecognized character in number",
    "Unrecognized character in expression",
    "Missing expression",
    "Divide by zero",
    "Unrecognized operator",
    "Non-constant label for org directive",
    "Can't use expression for org directive",
    "Missing colon at end of address label or unrecognized instruction",
    "Bad equ directive statement",
    "Bad db/dw directive statement",
    "Bad character constant"
  };

/* All labels have to start with an alpha char; Afterwards may have any 
 * alphanumeric char plus '_'.
 */
#define isLabel(x) isLabel_table[(int) x]

static const int isLabel_table[ASCII_MAX] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  !"#$%&'()*+,-./ */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 0123456789:;<=>? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* @ABCDEFGHIJKLMNO */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, /* PQRSTUVWXYZ[\]^_ */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* `abcdefghijklmno */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0  /* pqrstuvwxyz{|}~  */
 };

/* A letter ending a number may define that numbers base.
 * Currently b, h, o, and d (optional for decimal).
 */
#define isBase(x) isBase_table[(int) x]

static const int isBase_table[ASCII_MAX] = 

  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  !"#$%&'()*+,-./ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0123456789:;<=>? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* @ABCDEFGHIJKLMNO */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* PQRSTUVWXYZ[\]^_ */
    0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, /* `abcdefghijklmno */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* pqrstuvwxyz{|}~  */
  };

/* ops contains all  existing supported mathematical 
 * operators in reverse order of precedence (follows C).
 */
const str_storage ops = ".|^&-+%/*";

/* isOp macro returns true if char is operator
 */
#define isOp(x) isOp_table[(int) x]

const int isOp_table[ASCII_MAX] = 
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, /*  !"#$%&'()*+,-./ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0123456789:;<=>? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* @ABCDEFGHIJKLMNO */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, /* PQRSTUVWXYZ[\]^_ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* `abcdefghijklmno */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0  /* pqrstuvwxyz{|}~  */
 };

/* isExpr macro returns true if char can legally be part
 * of an expression. alphanumeric, ops or parenthesis.
 */
#define isExpr(x) isExpr_table[(int) x]

static const int isExpr_table[ASCII_MAX] = 
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, /*  !"#$%&'()*+,-./ */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 0123456789:;<=>? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* @ABCDEFGHIJKLMNO */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, /* PQRSTUVWXYZ[\]^_ */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* `abcdefghijklmno */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0  /* pqrstuvwxyz{|}~  */
 };

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

/* Macros that return legal tokens from defn's
 * processor specific header file
 */
#define isCharToken(x) isCharToken_table[(int) x]

#define isToken(x) (isToken_table[(int) x] + isOp(x))

#define ASM_DIRCTV_LEN  5

const char *asm_dirctv[ASM_DIRCTV_LEN] = 
  {
    "db", "dw", "equ", "org"
  };

extern char *buffer;        /* line to be assembled */
extern int pos;             /* used by nextToken() for pos in buffer */

int memory[MEMORY_MAX] = { 0 }; /* 64K of program memory */
int pc = 0;                     /* location of next instruction to be assembled */

FILE *lst = NULL;               /* list file descriptor, none if NULL */
FILE *obj = NULL;               /* object file descriptor, none if NULL */

label_type *labels = NULL;      /* array of defined labels */
int num_labels;                 /* number of defined labels */

int *sort_labels = NULL;        /* array of sorted labels */
int size_buf_labels;            /* size of label buffer */

typedef const char* (*getStrFunc)(int);

static int binarySearch(const char* name, int *index, getStrFunc getName)
{
  const char *token;
  int i, msb, bit, step, result;

  if (!strcmp(name, getName(0)))
    {
      *index = 0; return 0;
    }
  
  for (msb = SRCH_BIT_MAX; msb>0; --msb) /* find 1st msb bit not out of bounds */
    {
      if (getName(1<<msb)) break;
    }
  i = 0;
  for (bit = msb; bit>=0; --bit)
    {
      step = 1<<bit;
      i += step;
      if ( (token = getName(i)) ) 
	{
	  result = strcmp(name, token);
	  if (!result) break; else if (result>0) continue;
	}
      i -= step;
    }
  *index = i;
  return result;
}

static const char* getDirctvName(int index)
{
 if (index<ASM_DIRCTV_LEN)
    {
      return asm_dirctv[index];
    }
  else
    {
      return NULL;
    }
}

static const char *getLabelName(int index)
{
  if (index<num_labels)
    {
      return labels[sort_labels[index]].name;
    }
  else
    {
      return NULL;
    }
}

/* nextToken reads buffer staring at position pos. It returns a substr
 * that is made up of legal characters for a token
 * Then it does a binary search of the array tokens.
 * If no match checks for label, number, and expr
 */
static int nextToken(char *token)
{
  int index, end;
  int start = pos; /* start where last call left off */

  while (buffer[start] && isspace(buffer[start])) ++start;

  /* return NULL if only comment or whitespace found */

  if (!buffer[start] || buffer[start]==';') return 0;

   if (buffer[start]==',') 
     { 
       pos = start+1; return comma;
     }
   if (buffer[start]=='\'')
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

  /* If char is not token or not part of one, return error */

  if (!isCharToken(buffer[start]) && !isToken(buffer[start])) longjmp(err, no_char);

  if (isCharToken(buffer[start]))
    {
      pos = start + 1;
      token[0] = buffer[start]; token[1] = '\0';
    }
  else
    {
      end = start + 1;
      while (isToken(buffer[end])) ++end;
      pos=end;
      strncpy(token, buffer+start, end - start); token[end - start] = '\0';
    }
  
  index = 0;
  while (token[index]) 
    {
      token[index] = tolower(token[index]); ++index;
    }


  if (!binarySearch(token, &index, getToken))
    {
      return index + 1;       /* found match, first token is 1, not 0 */
    }

  if (!binarySearch(token, &index, getDirctvName))
    {
      return index + DIRCTV_BASE;
    }

  if (isalpha(token[0]))         /* not instr, but if 1st char alpha, label? */
    {
      for (index = 1; token[index] && isLabel(token[index]); ++index);
      if (token[index]==':' && !token[index+1]) 
	{
	  token[index] = '\0';
	  return addr_label;
	}
      if (!token[index]) return label;
    }

  if (isdigit(token[0]))       /* number can be a digit plus alphanumeric str */
    {
      for (index = 0; token[index] && isalnum(token[index]); ++index);
      if (!token[index]) return number; 
    }

  return expr;  /* has to be expr, will check syntax later */
}

static int setLabel(char *name, int value, int overWrite)
{
  int index;

  if (binarySearch(name, &index, getLabelName))
    {
      ++index;
      labels[num_labels].name[LBL_LEN_MAX] = '\0';
      strncpy(labels[num_labels].name, name, LBL_LEN_MAX);

      if (num_labels+1>LBL_MAX)
	{
	  printf("Can't define more labels.\n"); exit(1);
	}
      if (num_labels+1>size_buf_labels)
	{
	  size_buf_labels += LBL_BUF;
	  safeRealloc(labels, label_type, size_buf_labels);
	  safeRealloc(sort_labels, int, size_buf_labels);
	}
      
      memmove((void*) (sort_labels + index + 1), (void*) (sort_labels  + index), 
	      (num_labels - index)*sizeof(label_type));
      sort_labels[index] = num_labels;
      ++num_labels;      
    }
  else if (!overWrite)
    return sort_labels[index];

  labels[sort_labels[index]].value = value;
  return sort_labels[index];
}

int getLabelValue(char *name)
{
  int index;
  int value;

  if (binarySearch(name, &index, getLabelName))
    {
      longjmp(err, undef_label);
    }
  else
    {
      value = labels[sort_labels[index]].value;
      if (value == UNDEF) longjmp(err, labval_undef);
      return value;
    }
}

void printLabels(FILE* lst)
{
  int i;
  for (i=0; i<num_labels; ++i) 
    {
      fprintf(lst, "%s = %d\n", labels[sort_labels[i]].name,
	      labels[sort_labels[i]].value);
    }
}

static int getNumBase(char *b, int base)
{
  int value = 0;
  int digit;
  while (*b)
    {
      digit = (isdigit(*b)) ? *b - '0' : *b - 'a' + 10;
      if (digit>=base) longjmp(err, bad_number);

      value = value*base + digit;
      ++b;
    }
  return value;
}

static int getNumber(char *number)
{
  int value = 0;
  char b = number[strlen(number) - 1];
  if (number[0] == '0' && number[1] == 'x')
    {
      value = getNumBase(number + 2, 16);
    }
  else if (isalpha(b))
    {
      number[strlen(number) - 1] = '\0';
      switch (b)
	{
	case 'b': value = getNumBase(number, 2);  break;
	case 'd': value = getNumBase(number, 10); break;
	case 'h': value = getNumBase(number, 16); break;
	case 'o': value = getNumBase(number, 8);  break;
	}
    }
  else if (number[0] == '0')
    {
      value = getNumBase(number+1, 8);
    }
  else
    value = getNumBase(number, 10);
  return value;
}

/* findOp will search for an operator in expr str.
 * Note: this routine skips over paranthetical expr's.
 */
static int findOp(char op, char *expr)
{
  int level, p = 0;

  while (expr[p])
    {
      if (expr[p] == '(') /* don't return ops in paran expr's */
	{
	  level = 1;
	  while (level)   /* skip until back to level 0 */
	    {
	      if (expr[p] == '(')
		{
		  ++level;
		}
	      else if (expr[p] == ')')
		{
		  --level;
		}
	      ++p;
	    }
	}
      else if (isalpha(expr[p]))
	{
	  while (isLabel(expr[++p])); /* skip over labels */
	}
      else if (isdigit(expr[p]))
	{
	  while (isalnum(expr[++p])); /* skip over numbers */
	}
      else if (isspace(expr[p]))
	{
	  ++p;                        /* skip over whitespace */
	}
      else if (expr[p] == op)
	{
	  return p;                  /* return pos of op */
	}
      else if (!isOp(op))
	{
	  longjmp(err, bad_expr);      /* unknown char */
	}
      else
	++p;
    }
  return -1;
}

static int getExpr(char *expr)
{
  int pos, p = 0;
  int lvalue = 0;
  int rvalue = 0;
  int value = 0;
  char subexpr[BUF_SIZE];

  while (isspace(expr[0])) ++expr;
  p = strlen(expr)-1;
  while (p>=0 && isspace(expr[p])) --p;
  if (p<0) longjmp(err, miss_expr);
  expr[p+1]='\0';

  p = 0;
  while (ops[p])
    {
      pos = findOp(ops[p], expr+1);
      if (pos>0) break;
      ++p;
    }
  if (pos<0)
    {
      p = (expr[0] == '-');

      if (isalpha(expr[p]))
	{
	  value = getLabelValue(expr+p);
	}
      else if (isdigit(expr[p]))
	{
	  value = getNumber(expr+p);
	}
      if (p)
	return -value;
      else
	return value;
    }

  ++pos;
  strncpy(subexpr, expr, pos); subexpr[pos] = '\0';
  lvalue = getExpr(subexpr);
  
  strcpy(subexpr, expr+pos+1);
  rvalue = getExpr(subexpr);
     
  switch (expr[pos])
    {
    case '+': value = lvalue + rvalue; break;
    case '-': value = lvalue - rvalue; break;
    case '*': value = lvalue * rvalue; break;
    case '&': value = lvalue & rvalue; break;
    case '|': value = lvalue | rvalue; break;
    case '^': value = lvalue ^ rvalue; break;
    case '/': 
      if (!rvalue) longjmp(err, zero_div);
      value = lvalue / rvalue;
      break;
    default: 
      longjmp(err, no_op);
      break;
    }

  return value;
}

static void newdbtkn(int *dbtkn, int *tkn, int instr_token, int data_token)
{
  int dt = 0, t = 0;

  dbtkn[dt++] = instr_token;
  dbtkn[dt++] = 0;
  dbtkn[dt++] = 0;
  dbtkn[dt++] = 0;
  if (tkn[++t] != number) longjmp(err, bad_db);
  ++t;
  dbtkn[dt++] = data_token;
  
  while (tkn[++t])
    {
      if (tkn[t++] != comma) longjmp(err, bad_db);
      dbtkn[dt++] = comma;
      
      if (tkn[t++] != number) longjmp(err, bad_db);
      dbtkn[dt++] = data_token;
    }
  dbtkn[dt] = 0;
}

void firstPass(void)
{
  char token[BUF_SIZE];
  const int *theInstr;
  int tkn_pos = 0;
  int tkn[TKN_BUF] = { 0 };       /* tokenized buffer */


  while (tkn_pos<TKN_BUF-1 && (tkn[tkn_pos] = nextToken(token)) )
    {
      switch (tkn[tkn_pos])
	{
	case number:
	  tkn[++tkn_pos] = getNumber(token); 
	  break;
	case character:
	case expr:
	  tkn[++tkn_pos] = UNDEF; 
	  break;
	case label:
	  tkn[++tkn_pos] = setLabel(token, UNDEF, FALSE);
	  break;
	case addr_label: 
	  setLabel(token, pc, TRUE); 
	  --tkn_pos;
	  break;
	case equ:
	  if (nextToken(token) == number)
	    {
	      tkn[++tkn_pos] = number;
	      tkn[++tkn_pos] = getNumber(token);
	    }
	  else
	    continue; /* don't process equ expr's now */
	  break;
	}
      ++tkn_pos;
    }
  if (!tkn_pos) return;
  
  tkn[tkn_pos] = 0;
  switch (tkn[0])
    {
    case org:
      if (tkn[1] == number)
	pc = tkn[2];
      else if (tkn[1] == label)
	{
	  pc = labels[tkn[1]].value;
	  if (pc<0) longjmp(err, undef_org);
	}
      else
	longjmp(err, noexpr_org);
      break;
    case label:
      if (tkn[2] != equ) longjmp(err, miss_colon);
      if (tkn[3] == number)
	{
	  labels[tkn[1]].value = tkn[4];
	}
      else if (tkn[3] == label)
	{
	  labels[tkn[1]].value = labels[tkn[4]].value;
	}
      else if (tkn[3] != expr) /* only  number, label or expr after equ */
	longjmp(err, bad_expr);
      break;
    case db:
      pc+= tkn_pos/2;
      break;
    case dw:
      pc+= tkn_pos;
      break;
    default:
      theInstr = findInstr(tkn);
      pc += theInstr[INSTR_TKN_BYTES];
      break;
    }
}

void secondPass(void)
{
  char token[BUF_SIZE];
  int t;
  int tkn_pos = 0;
  int tkn[TKN_BUF] = { 0 };       /* tokenized buffer */
  int dbtkn[TKN_BUF] = { 0 };

  while (tkn_pos<TKN_BUF-1 && (tkn[tkn_pos] = nextToken(token)) )
    {
      switch (tkn[tkn_pos])
	{
	case number:
	  tkn[++tkn_pos] = getNumber(token); 
	  break;
	case character:
	  tkn[tkn_pos++] = number;
	  if (token[1] == '\\')
	    {
	      if ( (tkn[tkn_pos] = slashChar(token[2])) == 0) longjmp(err, bad_char);
	    }
	  else
	    {
	      tkn[tkn_pos] = token[1];
	    }
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
	    tkn[++tkn_pos] = setLabel(token, UNDEF, FALSE);
	  break;
	case addr_label:
	  --tkn_pos;
	  break;
	case equ:
	  while (isspace(buffer[pos])) ++pos; t = pos; /* read expr after equ */
	  while (isExpr(buffer[pos])) ++pos;
	  strncpy(token, buffer+t, pos - t); token[pos - t] = '\0';
	  tkn[++tkn_pos] = number;
	  tkn[++tkn_pos] = getExpr(token);
	  break;
	}
      ++tkn_pos;
    }
  
  if (tkn_pos)
    {
      tkn[tkn_pos] = 0;
      switch (tkn[0])
	{
	case org:
	  if (obj) writeObj(pc, tkn[2]);
	  pc = tkn[2];
	  break;
	case label:
	  if (tkn[2] == equ && tkn[3] == number  && !tkn[5]) 
	    {
	      labels[tkn[1]].value =  tkn[4];
	    }
	  else
	    {
	      longjmp(err, bad_equ);
	    }
	  break;
	case db:
	  newdbtkn(dbtkn, tkn, db, data_8);
	  loadMemory(dbtkn, tkn);
	  break;
	case dw:
	  newdbtkn(dbtkn, tkn, dw, data_16);
	  loadMemory(dbtkn, tkn);
	  break;
	default:
	  loadMemory(findInstr(tkn), tkn);
	  break;
	}
    }
}

int doPass(passFunc pass)
{
  int errNo, oldPC;
  int line = 0;
  int numErr = 0;

  initLabels();
  pc = 0;
  while (getBuffer())
    {
      ++line; oldPC = pc;
      if ((errNo = setjmp(err)) != 0)
	{
	  ++numErr;
	  if (lst) printErr(lst, errNo, line);
	  printErr(stderr, errNo, line);
	  continue;
	}
      pass();
      if (lst) writeList(oldPC, line, buffer);
    }
  return numErr;
}


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
    no_char = FRONTERR+1, undef_label, labval_undef, bad_number, bad_expr, zero_div, 
    no_op, undef_org, noexpr_org, miss_colon, bad_equ, bad_db, bad_char, miss_par, 
    no_leftPar, bad_addr
  };

const str_storage frontErrMsg[] = 
  {
    "Illegal character encountered", 
    "Undefined label",
    "Label value undefined",
    "Unrecognized character in number",
    "Unrecognized character in expression",
    "Divide by zero",
    "Unrecognized operator",
    "Non-constant label for org directive",
    "Can't use expression for org directive",
    "Missing colon at end of address label or unrecognized instruction",
    "Bad equ directive statement",
    "Bad db/dw directive statement",
    "Bad character constant",
    "Missing closing paranthesis",
    "Missing opening paranthesis",
    "Bad memory address"
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
static const str_storage binary_ops = "=|^&<>-+%/*";

static const str_storage unitary_ops = "~!+-";

/* isOp macro returns true if char is operator
 */
#define isOp(x) isOp_table[(int) x]

const int isOp_table[ASCII_MAX] = 
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, /*  !"#$%&'()*+,-./ */
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
    0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, /*  !"#$%&'()*+,-./ */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 0123456789:;<=>? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* @ABCDEFGHIJKLMNO */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, /* PQRSTUVWXYZ[\]^_ */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* `abcdefghijklmno */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0  /* pqrstuvwxyz{|}~  */
 };

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

/* assembly front end tokens
 */
#define ASM_DIRCTV_LEN  5

static const char *asm_dirctv[ASM_DIRCTV_LEN] = 
  {
    "db", "dw", "equ", "org"
  };

static char work_buf[BUF_SIZE]; /* working buffer */
extern char *buffer;            /* line to be assembled */
static int pos;                 /* used by nextToken() for pos in buffer */

int memory[MEMORY_MAX] = { 0 }; /* 64K of program memory */
int pc = 0;                     /* location of next instruction to be assembled */

int silent = FALSE;
char *filename = NULL;
FILE *lst = NULL;               /* list file descriptor, none if NULL */
FILE *obj = NULL;               /* object file descriptor, none if NULL */

label_type *labels = NULL;      /* array of defined labels */
int num_labels;                 /* number of defined labels */

int *sort_labels = NULL;        /* array of sorted labels */
int size_buf_labels;            /* size of label buffer */

/* binarySearch does a binary search of the sortted array of strings
 * that are accessed via the function getName. Returns zero if it 
 * finds string with index placed in *index. Otherwise, returns
 * -1 and index of string just before it in sort.
 */
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

/* helper function for searching pre-sorted front end tokens
 */
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

/* helper functions searching thru sorted list of labels
 * sort_labels is array of sorted indexes of unsorted array labels
 */
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

/* nextToken reads buffer staring at position pos. The token
 * found is placed in character array token.
 * First it looks for comma, character constant, single char token,
 * token, or label. If none matches it returns possible expr
 */
static int nextToken(char *token, const char* buffer)
{
  int d, index, end;
  int start = pos; /* start where last call left off */

  while (isspace(buffer[start])) ++start;

  if (!buffer[start]) return 0;

   if (buffer[start]==',') 
     { 
       pos = start+1; return comma;
     }
   if (buffer[start]=='\'') /* look for char constant */
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

  if (isCharToken(buffer[start])) /* look for single char tokens before all others */
    {
      pos = start + 1;
      token[0] = buffer[start]; token[1] = '\0';
      binarySearch(token, &index, getToken);
      return index + 1;
    }
  
  if (!isdigit(buffer[start]) && isToken(buffer[start])) /* possible token, addr label, or label */
    {
      end = start + 1;
      while (buffer[end] && (isToken(buffer[end]))) ++end;
      strncpy(token, buffer+start, end - start); token[end - start] = '\0';

      /* if we find a token or a label there may be a space or colon char
       * we need to skip over
       */
      pos=end + (buffer[end]==' ' || buffer[end]==':');
      
      if (!binarySearch(token, &index, getToken))
	{
	  return index + 1;        /* found match, first token is 1, not 0 */
	}
      
      if (!binarySearch(token, &index, getDirctvName))
	{
	  return index + DIRCTV_BASE;
	}
      
      if (isalpha(token[0]))            /* possible label */
	{
	  index = 0;
	  while (token[index])          /* check to see if token is legal label */
	    {
	      if (!isLabel(token[index])) break;
	      ++index;
	    }
	  if (!token[index])            /* if at end of string, legal label */
	    {
	      if (buffer[end]==':')     /* if followed by colon, addr label */
		{
		  return addr_label;
		}
	      else if (!buffer[end])
		{
		  return label;
		}
	      else if (buffer[end])    /* ordinary label not followd by arith op */
		{
		  while (buffer[end+1] && isspace(buffer[++end]));
		  if (!isOp(buffer[end])) return label;
		}
	    }
	}
    }

  /* find expression substring. Add to substring as longs as 
   * legal expression char or embedded space
   */
  end = start + 1;
  while (buffer[end] && (isExpr(buffer[end]) || isspace(buffer[end]))) ++end;
  
  d = 0;
  for (index=start; index<end; ++index) /* put expression substr in token, remove all spaces */
    {
      if (!isspace(buffer[index])) token[d++] = buffer[index];
    }
  token[d] = '\0';
  pos = end;

  if (isdigit(token[0]))
    {
      index = 0;
      while (token[index])          /* check to see if token could be simply a number */
	{
	  if (!isalnum(token[index])) break;
	  ++index;
	}
      if (!token[index]) return number; /* could be number, check syntax later */
    }
  return expr;  /* has to be expr, will check syntax later */
}

/* if label name does not exists, create label and set its value.
 * if label name already exists change its value to value if overWrite is TRUE.
 */
static int setLabel(const char *name, int value, int overWrite)
{
  int index;

  if (binarySearch(name, &index, getLabelName))
    {
      if (num_labels+1>LBL_MAX) /* binarySearch will only search LBL_MAX number of labels */
	{
	  printf("Can't define more labels.\n"); exit(1);
	}
      if (num_labels+1>size_buf_labels) /* out of space, realloc more memory for labels */
	{
	  size_buf_labels += LBL_BUF;
	  safeRealloc(labels, label_type, size_buf_labels);
	  safeRealloc(sort_labels, int, size_buf_labels);
	}
      labels[num_labels].name[LBL_LEN_MAX] = '\0';
      strncpy(labels[num_labels].name, name, LBL_LEN_MAX); /* create new label */
      
      /* index is location where new label should be placed in sorted list
       * Push up rest of array by 1 and place index of new label
       */
      ++index;
      memmove((void*) (sort_labels + index + 1), (void*) (sort_labels  + index), 
	      (num_labels - index)*sizeof(int));
      sort_labels[index] = num_labels;
      ++num_labels;      
    }
  else if (!overWrite)
    return sort_labels[index]; /* don't update value of label */

  labels[sort_labels[index]].value = value; /* update value of label */
  return sort_labels[index];                /* return its locations in array labels */
}

/* get value of label name. Causes fatal error if 
 * label does not exist or has undefined value
 */
int getLabelValue(const char *name)
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

/* print out all labels to file handler list
 */
void printLabels(FILE* lst)
{
  int i;
  for (i=0; i<num_labels; ++i) 
    {
      fprintf(lst, "%s = %d\n", labels[sort_labels[i]].name,
	      labels[sort_labels[i]].value);
    }
}

/* getNumBase reads alphanumberic string as a number of 
 * a given base. let is terminating character of number
 * to be ignored.
 */
static int getNumBase(const char *b, int base, char let)
{
  int value = 0;
  int digit;
  while (*b && *b != let)
    {
      digit = (isdigit(*b)) ? *b - '0' : *b - 'a' + 10;
      if (digit>=base) longjmp(err, bad_number);

      value = value*base + digit;
      ++b;
    }
  return value;
}

/* getNumber will attempt to parse string number as a numerical
 * constant. Numbers beginning with "0x" will be interpeted as 
 * hex. A number that begins with '0' will be interpeted as ocatal.
 * A number that begins with a number and ends with 'b', 'd', 'h'
 * or 'o' will be interpeted as binary, decimal, hex or octal,
 * respectively. A simple number is by default decimal.
 */
static int getNumber(const char *number)
{
  int value = 0;
  char b = number[strlen(number) - 1];
  if (number[0] == '0' && number[1] == 'x')
    {
      value = getNumBase(number + 2, 16, '\0');
    }
  else if (isalpha(b))
    {
      switch (b)
	{
	case 'b': value = getNumBase(number, 2,  b); break;
	case 'd': value = getNumBase(number, 10, b); break;
	case 'h': value = getNumBase(number, 16, b); break;
	case 'o': value = getNumBase(number, 8,  b); break;
	}
    }
  else if (number[0] == '0')
    {
      value = getNumBase(number+1, 8, '\0');
    }
  else
    value = getNumBase(number, 10, '\0');
  return value;
}

/* this routine will search expression string expr until to finds left paranthesis 
 * that matches the right paranthesis at expr[pos-1]. Intermediate (..) pairs are 
 * skipped over. Fatal error is generated if right par is not found
 */
static int findClosePar(const char *expr, int pos)
{
  int level = 1;
  while (level && expr[++pos])   /* skip until back to level 0 */
    {
      if (expr[pos] == '(')
	{
	  ++level;
	}
      else if (expr[pos] == ')')
	{
	  --level;
	}
    }
  if (level) longjmp(err, miss_par);
  return pos;
}

/* findOp will search for an operator in expr str.
 * Note: this routine skips over paranthetical expr's.
 */
static int findOp(const char op, const char *expr)
{
  int p = 0;

  while (expr[p])
    {
      if (expr[p] == '(') /* don't return ops in paran expr's */
	{
	  p = findClosePar(expr, p) + 1;
	}
      else if (isalpha(expr[p]))
	{
	  while (isLabel(expr[++p])); /* skip over labels */
	}
      else if (isdigit(expr[p]))
	{
	  while (isalnum(expr[++p])); /* skip over numbers */
	}
      else if (expr[p] == op)
	{
	  return p;                  /* return pos of op */
	}
      else if (isOp(expr[p]))
	{
	  ++p;
	}
      else
	{
	  longjmp(err, bad_expr);      /* unknown char */
	}
    }
  return UNDEF;
}

/* getExpr returns the value of the expression given it. All labels should
 * exists with defined values. Division by zero will generate fatal error.
 * Local var subexpr is used to pass substrings to recursive getExpr calls.
 */
int getExpr(const char *expr)
{
  int pos, p, start, lvalue, rvalue, value;
  char subexpr[BUF_SIZE];

  /* if expression is (..), strip off paren's and return value 
   */
  if (expr[0]=='(' && !expr[findClosePar(expr, 0)+1])
    {
      strncpy(subexpr, expr+1, strlen(expr)-2); /* only strip par's if  matching   */
      subexpr[strlen(expr)-2] = '\0';           /* right par at end of string expr */
      return getExpr(subexpr);
    }
  else if (findOp(')', expr)>0)
    longjmp(err, no_leftPar);

  /* check if expression begins with unitary operator.
   */ 
  p = 0;
  while (unitary_ops[p]) { if (expr[0] == unitary_ops[p++]) break; }
  start = (unitary_ops[p] != '\0');

  /* look for lowest precedence arith operator not inside paranthesis
   */
  p = 0;
  while (binary_ops[p])
    {
      pos = findOp(binary_ops[p], expr+start) + start;

      if (pos>0) break;
      ++p;
    }

  /* if no operator found, expr is either address value, label or number.
   * Skip over leading unitary operator.
   */
  if (pos<0)
    {
      if (isalpha(expr[start]))
	{
	  value = getLabelValue(expr+start);
	}
      else if (isdigit(expr[start]))
	{
	  value = getNumber(expr+start);
	}
      /*      else if (expr[start] == '$')
	{
	  strcpy(subexpr, expr+start+1);
	  if (subexpr[strlen(subexpr)-1] == ':') longjmp(err, bad_addr);
	  if (subexpr[strlen(subexpr)-2] == ':')
	    {
	      subexpr[strlen(subexpr)-2] == '\0';
	      value = getMemory(getExpr(subexpr), subexpr[strlen(subexpr)-1]);
	    }
	  else
	    {
	      value = getMemory(getExpr(subexpr), '\0');
	    }
	  if (value == UNDEF) longjmp(err, bad_addr);
	  }*/
      else
	{
	  longjmp(err, no_op); /* unknown opeator???? */
	}
      if (!start) return value;

      switch (expr[0])
	{
	case '+': return value; break;
	case '-': return -value; break;
	case '!': return !value; break;
	case '~': return ~value; break;
	default: 
	  longjmp(err, no_op); /* unknown opeator???? */
	  break;
	}
    }

  /* operator was found, expr = exprLeft op exprRight.  Get value of
   * exprLeftand exprRight by extrating substr and calling getExpr
   */
  strncpy(subexpr, expr, pos); subexpr[pos] = '\0';
  lvalue = getExpr(subexpr);
  
  if (expr[pos] == '=') ++pos;
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
    case '>': value = lvalue > rvalue; break;
    case '<': value = lvalue < rvalue; break;
    case '/': 
      if (!rvalue) longjmp(err, zero_div);
      value = lvalue / rvalue;
      break;
    case '%':
      if (rvalue)
	value = lvalue % rvalue;
      else
	return 0;
      break;
    case '=':
      switch (expr[pos-1])
	{
	case '>':  value = lvalue >= rvalue; break;
	case '<':  value = lvalue <= rvalue; break;
	case '!':  value = lvalue != rvalue; break;
	case '=':  value = lvalue == rvalue; break;
	default: 
	  longjmp(err, no_op); /* unknown opeator???? */
	  break;
	}
      break;
    default: 
      longjmp(err, no_op); /* unknown opeator???? */
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

void firstPass(const char *buffer)
{
  char token[BUF_SIZE];
  const int *theInstr;
  int tkn_pos = 0;
  int tkn[TKN_BUF] = { 0 };       /* tokenized buffer */


  while (tkn_pos<TKN_BUF-1 && (tkn[tkn_pos] = nextToken(token, buffer)) )
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
	  if (nextToken(token, buffer) == number)
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

void secondPass(const char *buffer)
{
  char token[BUF_SIZE];
  int tkn_pos = 0;
  int tkn[TKN_BUF] = { 0 };       /* tokenized buffer */
  int dbtkn[TKN_BUF] = { 0 };

  while (tkn_pos<TKN_BUF-1 && (tkn[tkn_pos] = nextToken(token, buffer)) )
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
	  if (!nextToken(token, buffer)) longjmp(err, bad_equ);
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
  int errNo, oldPC = 0, s, d, line = 0, numErr = 0;

  initLabels();
  pc = 0;
  while ( (getBuffer()) )
    {
      ++line;
      s = 0; d = 0;
      while (isspace(buffer[s])) ++s;
      while (buffer[s] && buffer[s] != ';')
	{
	  if (isToken(work_buf[d-1]) && buffer[s] && isToken(buffer[s])) work_buf[d++] = ' ';	  
	  while (buffer[s] && !isspace(buffer[s])) work_buf[d++] = tolower(buffer[s++]);
	  while (isspace(buffer[s])) ++s;
	}
      work_buf[d] = '\0';
      if (d)
	{
	  oldPC = pc; pos = 0;
	  if ((errNo = setjmp(err)) != 0)
	    {
	      ++numErr;
	      if (lst) printErr(lst, filename, errNo, line);
	      printErr(stderr, filename, errNo, line);
	      continue;
	    }
	  pass(work_buf);
	}
      if (lst) writeList(oldPC, line, buffer);
    }
  return numErr;
}


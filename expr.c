/*************************************************************************************

    Copyright (c) 2003 - 2005 by James L. Terman
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <memory.h>
#include <stdlib.h>
#include <setjmp.h>

/* This file handles functions related to labels and expressions
 */

#define EXPR_LOCAL

#include "asmdefs.h"
#include "asm.h"
#include "err.h"
#include "cpu.h"

const str_storage exprErrMsg[LAST_EXPR_ERR] = 
  {
    "Undefined label",
    "Label value undefined",
    "Unrecognized character in number",
    "Unrecognized character in expression",
    "Divide by zero",
    "Unrecognized operator",
    "Missing closing paranthesis",
    "Missing opening paranthesis",
    "Assignment operator '=' not allowed",
    "Unknown or illegal memory reference",
    "Unknown or illegal register"
  };

/* legal characters for labels
 */
const int isLabel_table[ASCII_MAX] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*  !"#$%&'()*+,-./ */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 0123456789:;<=>? */
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* @ABCDEFGHIJKLMNO */
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

/* recoginized unitary operators - lowest precedence
static const str_storage unitary_ops = "~!+-";
 */

/* single character operators
 */
const int isOp_table[ASCII_MAX] = 
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, /*  !"#$%&'()*+,-./ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, /* 0123456789:;<=>? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* @ABCDEFGHIJKLMNO */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, /* PQRSTUVWXYZ[\]^_ */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* `abcdefghijklmno */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0  /* pqrstuvwxyz{|}~  */
 };

/* characters that are legal in expressions
 */
const int isExpr_table[ASCII_MAX] = 
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

label_type *labels = NULL;       /* array of defined labels front.c can see */
static int *sort_labels = NULL;  /* array of sorted labels                  */
static int num_labels = 0;       /* number of defined labels                */
static jmp_buf *exprErr = &err;  /* ptr to global jmp_buf for error         */
static jmp_buf *lastErr = NULL;  /* jmp_buf variable to be restored         */

/* set jmp_buf variable for expr.c errors and save old value
 */
void setJmpBuf(jmp_buf* newErr)
{
  lastErr = exprErr;
  exprErr = newErr;
}

/* restore old set jmp_buf variable for expr.c errors
 */
void restoreJmpBuf(void)
{
  if (lastErr) exprErr = lastErr; lastErr = NULL;
}


/* helper func for bsearch to search through arrays of token strings
 */
int cmpstr(const void *e1, const void *e2)
{
  const char *s1 = e1; 
  char* const *s2 = e2;
  return strcmp(s1, *s2);
}

/* findlabel is a helper fuction for bsearch where e1 is the name of the
 * label to be found and e2 is a pointer to a index into labels
 */
static int findlabel(const void *e1, const void *e2)
{
  char *label = labels[*((const int*) e2)].name;
  return strcmp(e1, label);
}

/* cmplabel is a pointer function for qsort to sort an array of labels
 */
static int cmplabel(const void *e1, const void *e2)
{
  const int *i1 = e1, *i2 = e2;
  return strcmp(labels[*i1].name, labels[*i2].name);
}

/* handle any additions to label array. Keep sort_labels sorted.
 */
static int safeAddLabel(const char *name, int value)
{
  static int size_labels = 0;      /* size of label buffer     */
  int oldnum = num_labels;

  if (num_labels >= size_labels)
    {
      safeRealloc(labels, label_type, size_labels += CHUNK_SIZE);
      safeRealloc(sort_labels, int, size_labels);
    }
  labels[num_labels].value = value;
  labels[num_labels].name  = NULL;
  safeDupStr(labels[num_labels].name, name);
  sort_labels[num_labels] = num_labels;
  num_labels++;
  qsort(sort_labels, num_labels, sizeof(int), &cmplabel);
  return oldnum;
}

/* if label name does not exists, create label and set its value.
 * if label name already exists change its value to value if value UNDEF
 * or overWrite is TRUE.
 */
int setLabel(str_storage name, int value, int overWrite)
{
  int *index, new_label;
  
  index = bsearch(name, sort_labels, num_labels, sizeof(int), &findlabel);
  if (!index)
    new_label = safeAddLabel(name, UNDEF);   /* create new label */
  else if (!overWrite && labels[*index].value != UNDEF)
    return *index;         /* don't assign label more than once  */
  else
    new_label = *index;

  labels[new_label].value = value; /* update value of label */
  return new_label;                /* return its locations in array labels */
}

/* return pointer to label if defined otherwise NULL
 */
label_type *getLabel(str_storage name)
{
  int *index = bsearch(name, sort_labels, num_labels, sizeof(int), &findlabel);
  if (index) return labels + *index; else return NULL;
}

/* get value of label name. Causes fatal error if 
 * label does not exist or has undefined value
 */
int getLabelValue(str_storage name)
{
  label_type *label = getLabel(name);

  if (label)
    {
      if (label->value == UNDEF) longjmp(*exprErr, labval_undef);
      return label->value;
    }
  else
    longjmp(*exprErr, undef_label);
}

/* print out all labels to file handler list
 */
void printLabels(FILE* lst)
{
  int i;
  for (i=10; i<num_labels; ++i) 
    {
      fprintf(lst, "%s = %d\n", labels[sort_labels[i]].name,
	      labels[sort_labels[i]].value);
    }
}

/* getNumBase reads alphanumberic string as a number of 
 * a given base. let is terminating character of number
 * to be ignored.
 */
static int getNumBase(str_storage b, int base, char let)
{
  int value = 0;
  int digit;
  while (*b && *b != let)
    {
      digit = (isdigit(*b)) ? *b - '0' : *b - 'a' + 10;
      if (digit>=base) longjmp(*exprErr, bad_number);

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
int getNumber(str_storage number)
{
  int value = 0;
  char b = lastChar(number);

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
      value = getNumBase(number + 1, 8, '\0');
    }
  else
    value = getNumBase(number, 10, '\0');
  return value;
}

/* findClosePar() will search expression string expr+pos until to finds right
 * paranthesis that matches the left paranthesis at expr[pos-1]. Intermediate
 * (..) pairs are skipped over. Fatal error is generated if right par is not 
 * found
 */
int findClosePar(str_storage expr, int pos)
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
  if (level) longjmp(*exprErr, miss_par);
  return pos;
}

/* findOp will search for an operator in expr str.
 * Note: this routine skips over paranthetical expr's.
 */
static int findOp(const char op, str_storage expr)
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
      else
	{
	  ++p;
	}
    }
  return UNDEF;
}

/* findBinaryOp() willlook for lowest precedence arith operator 
 * not inside paranthesis in expr
 */
static int findBinaryOp(str_storage expr)
{
  int pos = 0, op = 0;
  while (binary_ops[op])
    {
      pos = findOp(binary_ops[op], expr);
      
      if (pos>0) break;
      ++op;
    }
  return pos;
}

/* getExpr returns the value of the expression given it. All labels should
 * exists with defined values. Division by zero will generate fatal error.
 * okay to change expr as long as you undo it before you undo it
 */
int getExpr(char *expr)
{
  int pos, start = 0, lvalue, rvalue, value, *mem;
  char op;

  /* check if expression begins with unitary operator.
   * if so, skip over them
   */ 
  start = 0;
  while (isOp(expr[start])) ++start;

  if (expr[start]=='(' && !expr[start + (pos = findClosePar(expr + start, 0)) + 1])
    { /* expr starts with ( and ends with ).
       */
      expr[start + pos] = '\0';
      value = getExpr(expr + start + 1);  /* get value of expr in ()'s */
      expr[start + pos] = ')';
    }
  else if (findOp(')', expr + start)>0)
    {
      /* error only occurs if there is an unmatched ')'
       */
      longjmp(*exprErr, no_leftPar);
    }
  else if ((pos = findBinaryOp(expr + start)) != UNDEF)
    {
      /* = char only found in == operator, can't be used alone
       */
      if (expr[start + pos] == '=' && expr[start + pos + 1] != '=') 
	longjmp(*exprErr, no_eq);
      
      /* OPerator was found: expr = exprLeft OP exprRight.  Get value of
       * exprLeft and exprRight by extracting substring and calling getExpr
       */
      op = expr[start + pos];
      expr[start + pos] = '\0';
      lvalue = getExpr(expr); /* note: leading unary ops included in lvalue */
      expr[start + pos] = op;
            
      /* 2 char operators are either >>, <<, == or single char op + '='
       */
      if ((expr[start + pos] == expr[start + pos + 1]) || 
	  (expr[start + pos + 1] == '=')) ++pos;
      rvalue = getExpr(expr + start + pos + 1);
      
      switch (expr[pos])
	{
	case '+': value = lvalue + rvalue; break;
	case '-': value = lvalue - rvalue; break;
	case '*': value = lvalue * rvalue; break;
	case '&': value = lvalue & rvalue; break;
	case '|': value = lvalue | rvalue; break;
	case '^': value = lvalue ^ rvalue; break;
	case '/': 
	  if (!rvalue) longjmp(*exprErr, zero_div);
	  value = lvalue / rvalue;
	  break;
	case '%':
	  value = (rvalue) ? lvalue % rvalue : 0;
	  break;      
	case '>': 
	  value = (expr[pos + 1] == '>') ? lvalue >> rvalue : lvalue > rvalue;
	  break;
	case '<':
	  value = (expr[pos + 1] == '<') ? lvalue << rvalue : lvalue < rvalue;
	  break;
	case '=':
	  switch (expr[pos - 1])
	    {
	    case '>':  value = lvalue >= rvalue; break;
	    case '<':  value = lvalue <= rvalue; break;
	    case '!':  value = lvalue != rvalue; break;
	    case '=':  value = lvalue == rvalue; break;
	    default: 
	      longjmp(*exprErr, no_op); /* unknown operator???? */
	      break;
	}
	  break;
	default: 
	  longjmp(*exprErr, no_op); /* unknown operator???? */
	  break;
	}
      return value; /* no more processing needed */
    }
  else
    { /* if no operator found, expr is either address value, label or number.
       * Skip over leading unitary operator.
       */
      if (isalpha(expr[start]))
	{
	  value = getLabelValue(expr + start);
	}
      else if (isdigit(expr[start]))
	{
	  value = getNumber(expr + start);
	}
      else if ((expr[start]=='$'|| expr[start]=='@'))
	{
	  if (!(mem = getMemExpr(expr + start)))
	    longjmp(*exprErr, (expr[start]) ? no_mem : bad_reg);
	  value = *mem;
	}
      else
	{
	  longjmp(*exprErr, no_op); /* unknown opeator???? */
	}
    }
  
  while (start--)
    {
      switch (expr[start])
	{
	case '+': value = +value; break;
	case '-': value = -value; break;
	case '!': value = !value; break;
	case '~': value = ~value; break;
	default: 
	  longjmp(*exprErr, no_op); /* unknown opeator???? */
	  break;
	}
    }
  return value;
}

/* this function will allocate memory for the label array and will 
 * add any predefined processor specific labels.
 */
void initLabels(void)
{
  int i;
  char label[3] = "@0";

  if (labels) return;

  for (i = 0; i<10; ++i)
    {
      label[1] = '0' + i;
      safeAddLabel(label, UNDEF);
    }
  i = 0;
  while (def_labels[i].value != UNDEF) 
    {
      safeAddLabel(def_labels[i].name, def_labels[i].value); ++i;
    }
}

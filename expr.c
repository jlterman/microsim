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

 /* array of cpu specific labels in asm.c
  */
extern const label_type def_labels[];

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

/* recoginized unitary operators - lowest precedence
static const str_storage unitary_ops = "~!+-";
 */

/* single character operators
 */
const int isOp_table[ASCII_MAX] = 
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, /*  !"#$%&'()*+,-./ */
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

label_type *labels = NULL;        /* array of defined labels front.c can see */
static int num_labels;            /* number of defined labels                */
static int size_labels;           /* size of label buffer                    */
static int *sort_labels = NULL;   /* array of sorted labels                  */

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

/* if label name does not exists, create label and set its value.
 * if label name already exists change its value to value if overWrite is TRUE.
 */
int setLabel(str_storage name, int value, int overWrite)
{
  int *index;
  int new_label = num_labels;
  
  index = bsearch(name, sort_labels, num_labels, sizeof(int), &findlabel);
  if (!index)
    {
      if (num_labels == size_labels) 
	{
	  /* out of space, realloc more memory for labels
	   */
	  size_labels += CHUNK_SIZE;
	  safeRealloc(labels, label_type, size_labels);
	  safeRealloc(sort_labels, int, size_labels);
	}
      ++num_labels;
      labels[new_label].name = strdup(name);    /* create new label */
      sort_labels[new_label] = new_label;

      qsort(sort_labels, num_labels, sizeof(int), &cmplabel);
    }
  else if (!overWrite) /* if overWrite false, don't update value of label */
    return *index;
  else
    new_label = sort_labels[index - sort_labels];

  labels[new_label].value = value; /* update value of label */
  return new_label;                /* return its locations in array labels */
}

/* get value of label name. Causes fatal error if 
 * label does not exist or has undefined value
 */
int getLabelValue(str_storage name)
{
  int *index, value;

  index = bsearch(name, sort_labels, num_labels, sizeof(int), &findlabel);
  if (index)
    {
      value = labels[*index].value;
      if (value == UNDEF) longjmp(err, labval_undef);
      return value;
    }
  else
    longjmp(err, undef_label);

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
static int getNumBase(str_storage b, int base, char let)
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
static int findClosePar(str_storage expr, int pos)
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

/* findBinaryOp() willlook for lowest precedence arith operator 
 * not inside paranthesis in expr
 */
static int findBinaryOp(str_storage expr)
{
  int pos, op = 0;
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
      longjmp(err, no_leftPar);
    }
  else if ((pos = findBinaryOp(expr + start)) != UNDEF)
    {
      /* = char only found in == operator, can't be used alone
       */
      if (expr[start + pos] == '=' && expr[start + pos + 1] != '=') 
	longjmp(err, no_eq);
      
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
	  if (!rvalue) longjmp(err, zero_div);
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
	      longjmp(err, no_op); /* unknown operator???? */
	      break;
	}
	  break;
	default: 
	  longjmp(err, no_op); /* unknown operator???? */
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
	  if ((mem = getMemExpr(expr + start)))
	    longjmp(err, (expr[start]) ? no_mem : bad_reg);
	  value = *mem;
	}
      else
	{
	  longjmp(err, no_op); /* unknown opeator???? */
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
	  longjmp(err, no_op); /* unknown opeator???? */
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

  if (labels) return;
  safeMalloc(labels, label_type, CHUNK_SIZE);
  safeMalloc(sort_labels, int, CHUNK_SIZE);
  
  i = 0;
  while (def_labels[i].value != UNDEF)
    {
      labels[i].name = strdup(def_labels[i].name);
      labels[i].value = def_labels[i].value;
      sort_labels[i] = i;
      ++i;
    }
  num_labels = i;
  size_labels = CHUNK_SIZE;
  qsort(sort_labels, num_labels, sizeof(int), &cmplabel);
}

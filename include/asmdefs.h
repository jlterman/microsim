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

/* These are common defintions for the entire project
 */

#ifndef _ASM_DEFS
#define _ASM_DEFS

/* label type definition
 */
typedef struct 
{
  char *name;
  int  value;
} label_type;

/* string storage type. Allows for const str_storage for predefined str arrays
 */
typedef const char* str_storage;

/* Global MACRO definitions used universally
 */
#define TRUE 1
#define FALSE 0

#define MEMORY_MAX 65536
#define BUFFER_SIZE 256
#define CHUNK_SIZE 1024

#define ASCII_MAX 128
#define BYTE_MAX 256
#define BYTE_MASK 0xFF
#define SGN_BYTE_MAX 127
#define SGN_BYTE_MIN -128

#define BIT0_MASK 1
#define BIT1_MASK 2
#define BIT2_MASK 4
#define BIT3_MASK 8
#define BIT4_MASK 0x10
#define BIT5_MASK 0x20
#define BIT6_MASK 0x40
#define BIT7_MASK 0x80

#define HI_NYBLE 0xF0
#define LO_NYBLE 0x0F

/* UNDEF is used when 0 is a legal value for a function return
 */
#define UNDEF -65536

/* All labels have to start with an alpha char; Afterwards may have any 
 * alphanumeric char plus '_'.
 */
#define isLabel(x) isLabel_table[(int) x]

/* isOp macro returns true if char is operator
 */
#define isOp(x) isOp_table[(int) x]

/* isExpr macro returns true if char can legally be part
 * of an expression. alphanumeric, ops or parenthesis.
 */
#define isExpr(x) isExpr_table[(int) x]

#ifndef EXPR_LOCAL
extern const int isLabel_table[];
extern const int isExpr_table[];
extern const int isOp_table[];
#endif

/* Generally usefull macros
 */
#define safeMalloc(var, type, size) \
  {  if (!var) free(var); \
     var = (type*) malloc((size)*sizeof(type)); \
     if (!var) { fprintf(stderr, "Out of memory!\n"); exit(1); } }

#define safeCalloc(var, type, size) \
  {  if (!var) free(var); \
     var = (type*) calloc(sizeof(type), size); \
     if (!var) { fprintf(stderr, "Out of memory!\n"); exit(1); } }

#define safeRealloc(var, type, size) \
  {  var = (type*) realloc(var, (size)*sizeof(type)); \
     if (!var) { fprintf(stderr, "Out of memory!\n"); exit(1); } }

#define safeDupStr(new, str) \
  {  safeMalloc(new, char, strlen(str)+1); strcpy(new, str); }

#define getLow(x) (((unsigned) x) & 0xFF)
#define getHigh(x) ((((unsigned) x)/0x100) & 0xFF)

#define lastChar(x) (x)[strlen(x) - 1]

#endif

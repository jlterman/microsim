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

/* This file serves as the interface of the assembler module.
 * Assembler module will call getBuffer() to get next line.
 * List file will be output to lst file descriptor and
 * object file will be output to obj file descriptor.
 *
 * call dopass(firstPass) and dopass(secondPass) to assemble file
 */

#ifndef _ASM
#define _ASM

#define TRUE 1
#define FALSE 0

#define MEMORY_MAX 65536
#define BUF_SIZE 256

#define ASCII_MAX 128
#define BYTE_MAX 255
#define SGN_BYTE_MAX 127
#define SGN_BYTE_MIN -128

#ifndef FRONTEND_LOCAL
extern
#endif
void printLabels(FILE*);

#ifdef FRONTEND_LOCAL
extern
#endif
void writeObj(int, int);

#ifdef FRONTEND_LOCAL
extern
#endif
int getBuffer(void);

#ifndef FRONTEND_LOCAL
extern
#endif
int getLabelValue(char*);

#ifndef FRONTEND_LOCAL
extern
#endif
void firstPass(void);

#ifndef FRONTEND_LOCAL
extern
#endif
void secondPass(void);

typedef void (*passFunc)();

typedef const char* str_storage;
 
#ifndef FRONTEND_LOCAL
extern
#endif
int doPass(passFunc);

#define LBL_LEN_MAX 32

typedef struct 
{
  char name[LBL_LEN_MAX+1];
  int  value;
  int  id;
} label_type;   /* name, value pairs for labels */

#ifndef FRONT_LOCAL
extern int memory[MEMORY_MAX]; /* 64K of program memory */
extern int pc;                 /* location of next instruction to be assembled */

extern FILE *lst;              /* list file descriptor, none if NULL */
extern FILE *obj;              /* object file descriptor, none if NULL */

extern label_type *labels;     /* array of defined labels */
extern int num_labels;         /* number of defined labels */

extern int *sort_labels;       /* array of sorted labels */
extern int size_buf_labels;    /* size of label buffer */
#endif

#define safeMalloc(var, type, size) \
  {  var = (type*) malloc(size*sizeof(type)); \
     if (!var) { fprintf(stderr, "Out of memory!\n"); exit(1); } }

#define safeRealloc(var, type, size) \
  {  var = (type*) realloc(var, size*sizeof(type)); \
     if (!var) { fprintf(stderr, "Out of memory!\n"); exit(1); } }

#endif

#define getLow(x) ((x) & 0xFF)
#define getHigh(x) (((x)/0x100) & 0xFF)

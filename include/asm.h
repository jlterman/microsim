/*************************************************************************************

    Copyright (c) 2003, 2004 by James L. Terman
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

#ifndef _ASM_HEADER
#define _ASM_HEADER

/******************************************************************************
 *
 *    The following function are call back functions to be defined by the
 *    file that is linked to the assembler
 *
 *****************************************************************************/

/*  writeobj is given a range of program memory that has been assembled and
 * is ready to be written to a file
 */
#ifndef MAIN_LOCAL
extern
#endif
void writeObj(int, int);

/* Called by assembler to get next line of assembly. return NULL at EOF
 */
#ifndef MAIN_LOCAL
extern
#endif
str_storage getBuffer(void);

/* function called by assembler to report error found.
 * Gives error number, line of file and appropiate error message
 */
#ifndef MAIN_LOCAL
extern
#endif
void printErr(int, int, str_storage);

/* function called by assembler to try to handle memory reference
 * If returns UNDEF, will generate error message
 */
#ifndef MAIN_LOCAL
extern
#endif
int *getMemExpr(char*);

/******************************************************************************
 *
 *    The following function are global functions of the assembler
 *
 *****************************************************************************/

/* typedef of assembler pass function
 */
typedef void (*passFunc)(str_storage);
 
/* main entry point into assembler. Give pass function as assembler
 */
#ifndef FRONTEND_LOCAL
extern
#endif
int doPass(passFunc);

/* first pass of assember. No code is generated, but syntax is checked 
 * and all labels will be defined at end of pass for second pass
 */
#ifndef FRONTEND_LOCAL
extern
#endif
void firstPass(str_storage);

/* With all labels defined by first pass, second pass will be able to 
 * generate code. Range checking errors will be reported in this pass
 */
#ifndef FRONTEND_LOCAL
extern
#endif
void secondPass(str_storage);

/* will return value of expression passed to it consisting of operators, 
 * numerical constants and label values
 */
#ifndef EXPR_LOCAL
extern
#endif
int getExpr(char*);

/* getNumber() will interpet the string given to it as a numerical constant
 */
#ifndef EXPR_LOCAL
extern
#endif
int getNumber(str_storage);

/* print name and value of all defined labels
 */
#ifndef EXPR_LOCAL
extern
#endif
void printLabels(FILE*);

/* get value of label name passed as parameter
 */
#ifndef EXPR_LOCAL
extern
#endif
int getLabelValue(str_storage);

/* if label name does not exists, create label and set its value.
 */
#ifndef EXPR_LOCAL
extern
#endif
int setLabel(str_storage, int, int);

/* initialize labels for assembler
*/
#ifndef EXPR_LOCAL
extern
#endif
void initLabels(void);

/* function useful for searching an array of sorted str's with bsearch
 */
#ifndef EXPR_LOCAL
extern
#endif
int cmpstr(const void*, const void*);

#endif

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

#ifndef ERR_LOCAL
extern
#endif
void printErr(FILE*, const char*, int, int);

#ifndef ERR_LOCAL
extern
#endif
jmp_buf err;                    /* used for error handling */

#ifdef ERR_LOCAL
extern char *buffer;
extern const str_storage frontErrMsg[];
extern const str_storage  backErrMsg[];
extern const str_storage  procErrMsg[];
#endif

#define FRONTERR 0
#define BACKERR  256
#define CPUERR   512

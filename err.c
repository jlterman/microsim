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
#include <stdlib.h>
#include <setjmp.h>

#define ERR_LOCAL
#include "asm.h"
#include "err.h"

void printErr(FILE *fd, int errNo, int line)
{
  const char *msg;

  if (errNo<BACKERR) msg = frontErrMsg[errNo- FRONTERR - 1];
  else if (errNo<CPUERR) msg = backErrMsg[errNo - BACKERR -1];
  else msg = procErrMsg[errNo - CPUERR -1];

  fprintf(fd, "****** Syntax error #%d at line %d: %s\n", errNo, line, msg);
  fprintf(fd, "%s\n", buffer);
}

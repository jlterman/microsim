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
#include <getopt.h>
#include <string.h>

#ifdef __WIN32__
#include <io.h>
#endif

#define MAIN_LOCAL
#include "asmdefs.h"
#include "asm.h"
#include "cpu.h"
#include "version.h"

extern char cpu_version[];

const char asm_version[] = "Assembler " ASM_VERS "\nCopyright Jim Terman 2003";

FILE *lst = NULL;            /* file descriptor for assembly listing */

static char *filename;       /* name of file being assembled            */
static FILE *bufd;           /* file descripter of file being assembled */
static char *buffer = NULL;  /* line being assembled */
static FILE *obj = NULL;     /* file descriptor of object file */

/* this routine called to write size bytes at memory addr to a file. 
 * This writes out memory in the Intel Hex Format
 */

static void writeMemory(int addr, int size)
{
  int p;
  int checksum = size + getLow(addr) + getHigh(addr);

  fprintf(obj, ":%02X%04X00", size, addr);
  for (p = 0; p<size; ++p) 
    {
      fprintf(obj, "%02X", memory[addr+p]);
      checksum +=  memory[addr+p];
    }
  fprintf(obj, "%02X\n", getLow(0 - getLow(checksum)));
}

/* Global function called by assembly back end to write memory to a file
 * between curPC and newPC. writeMemory() will be called with a max of 16 bytes
 */
#define BYTES_PER_LINE 16

void writeObj(int curPC, int newPC)
{
  int i;
  int lines;
  int bytes;
  static int lastPC = 0;

  if (!obj) return;
  if (curPC>lastPC)
    {
      lines = (curPC - lastPC)/BYTES_PER_LINE;
      for (i = 0; i<lines; ++i) writeMemory(lastPC + i*BYTES_PER_LINE, BYTES_PER_LINE);
      bytes = (curPC - lastPC) % BYTES_PER_LINE;
      if (bytes) writeMemory(lastPC + lines*BYTES_PER_LINE, bytes);
    }
  lastPC = newPC;
  if (newPC == MEMORY_MAX) fprintf(obj, ":00000001FF\n");
}

/* print error message to file fd. Error format should be recognized.
 */
static void fprintErr(FILE *fd, int errNo, int line, str_storage msg)
{
  fprintf(fd, "%s:%d ****** Syntax error #%d: %s\n", filename, line, errNo, msg);
  if (buffer) fprintf(fd, "%s\n", buffer);
}

/* global function called by assembler when error is reported
 */
void printErr(int errNo, int line, str_storage msg)
{
  fprintErr(stderr, errNo, line, msg);
  if (lst && lst!=stdout) fprintErr(lst, errNo, line, msg);
}

/* global function called by assembler for memory reference
 * return NULL as this is not allowed in assembler
 */
int *getMemExpr(char *expr)
{
  return NULL;
}

/* return with a pointer to a file or exit program
 */
static FILE *safeOpen(char* filename, char* mode)
{
  FILE *fd;

  if (!filename) return NULL;
  if (!strcmp(filename, "-")) return stdin;
  if ( (fd = fopen(filename, mode)) == NULL )
    {
      fprintf (stderr, "Cannot open %s\n", filename);
      exit(1);
    }
  return fd;
}

/* global function called by assembly front end whenever it needs a new line
 * This getBuffer reads new line from file but will not return more than 256
 * bytes of line
 */
str_storage getBuffer(void)
{
  static char buf[BUFFER_SIZE];
  int nextLine, c;

  if (feof(bufd)) return NULL;
  if (!fgets (buf, BUFFER_SIZE, bufd)) return NULL;
  nextLine = (lastChar(buf) == '\n');
  safeDupStr(buffer, buf);
  while (!nextLine && !feof(bufd) && fgets (buf, BUFFER_SIZE, bufd))
    {
      safeRealloc(buffer, char, strlen(buffer) + strlen(buf) + 1);
      strcat(buffer, buf);
      nextLine = (lastChar(buffer) == '\n');
    }

  c = strlen(buffer);
  while (c && buffer[--c] < '!') buffer[c] = '\0';
  return buffer;
}

/* will replace the suffix ".xxx" in string oldname with the 
 * new suffix in newsuffix
 */
static char *newSuffix(char *oldname, char *newsuffix)
{
  char* newstr;

  int c = strrchr(oldname, '.') - oldname;
  if (!c) c = strlen(oldname);

  safeMalloc(newstr, char, c + 2 + strlen(newsuffix));
  strncpy(newstr, oldname, c + 1); newstr[c + 1] = '\0';
  strcpy(newstr + c +1, newsuffix);
  newstr[c] = '.';
  return newstr;
}

static void printHelp(void)
{
  printf("asm [-vVsLl] [-o file.obj] file1 file2 ...\n");
  exit(1);
}


int main(int argc, char *argv[])
{
  int c;
  int lstFlag = FALSE;
  char *lstFile = NULL;
  char *objFile = NULL;
  int numErr = 0;
  int silent = FALSE;
  int batch = FALSE;
  int verbose = FALSE;
  int version = FALSE;
  char temp[] = "asmXXXXXX";

  while ((c = getopt(argc, argv, "svhlLo:")) != EOF)
    {
      switch (c)
	{
	case 'l':
	  lstFlag = TRUE;
	  break;
	case 'o':
	  objFile = optarg;
	  break;
	case 'L':
	  batch = TRUE;
	  break;
	case 'V':
	  version = TRUE;
	  break;
	case 'v':
	  verbose = TRUE;
	  break;
	case 's':
	  silent = TRUE;
	  break;
	default:
	  printHelp();
	  break;
  
	}
    }
  if (optind == argc) printHelp();

  for (c = optind; c<argc; ++c)
    {
      filename = argv[c];
      if (lstFlag) lstFile = newSuffix(filename, "lst");
      bufd = safeOpen(filename, "r");
      if (batch)
	{
	  numErr += doPass(&firstPass);
	  rewind(bufd);
	  lst = stdout;
	  numErr += doPass(&secondPass);
	  if (numErr) printf("Assembly terminated with %d errors.\n", numErr);
	  return (numErr>0);
	}
      
      if (!objFile) objFile = newSuffix(filename, "obj");
      
#ifdef __WIN32__
      mktemp(temp);
      obj = safeOpen(temp, "w");
#else
      if ( (obj = fdopen(mkstemp(temp), "w") ) == NULL) /* mkstemp safer for unix systems */
	{ 
	  fprintf(stderr, "Can't open tmp file!\n"); exit(1); 
	}
#endif
      
      if (version) 
	printf("%s\n%s\nThis program is distributed under the GNU Public License.\n\n", cpu_version, asm_version);
      if (verbose && !strcmp(filename, "-"))  printf("Starting assembly from standard input\n");
      if (verbose) printf("Starting assembly of file %s\n", filename);
      
      numErr += doPass(&firstPass);
      if (!numErr && !batch && verbose) printf("First pass successfully completed.\n");
      
      rewind(bufd);
      lst = safeOpen(lstFile, "w");
      numErr += doPass(&secondPass);
      if (numErr)
	{
	  if (verbose) printf("Assembly terminated with %d errors.\n", numErr);
	  remove(temp);
	}
      else
	{
	  if (verbose) printf("Second pass successfully completed.\n\n");
	  if (lst) printLabels(lst);
	  
	  if (obj) writeObj(pc, MEMORY_MAX);
	  fclose(obj);
	  rename(temp, objFile);
	  
	  if (verbose) printf("Assembly sucessfully completed.\n"
			      "Object code written to file %s\n", objFile);
	  if (lst) printf("List output written to file %s\n", lstFile);
	}
      if (numErr>0) return numErr;
    }
  return 0;
}

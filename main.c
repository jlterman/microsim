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
#include "version.h"
#include "asm.h"

#ifdef __WIN32__
#include <io.h>
#endif

extern char cpu_version[];

const char asm_version[] = "Assembler " ASM_VERS "\nCopyright Jim Terman 2003";

static FILE *bufd;                  /* file descripter of file being read for buffer */
static char buf_store[BUF_SIZE];

char *buffer = buf_store;           /* line to be assembled */


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

#define BYTES_PER_LINE 16

void writeObj(int curPC, int newPC)
{
  int i;
  int lines;
  int bytes;
  static int lastPC = 0;

  if (curPC>lastPC)
    {
      lines = (curPC - lastPC)/BYTES_PER_LINE;
      for (i = 0; i<lines; ++i) writeMemory(lastPC+i*BYTES_PER_LINE, BYTES_PER_LINE);
      bytes = (curPC - lastPC) % BYTES_PER_LINE;
      if (bytes) writeMemory(lastPC+lines*BYTES_PER_LINE, bytes);
    }
  lastPC = newPC;
  if (newPC == MEMORY_MAX) fprintf(obj, ":00000001FF\n");
}

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

int getBuffer(void)
{
  static int complete = TRUE;
  int i;

  while (!complete)
    {
      fgets (buffer, BUF_SIZE, bufd); /* if complete line was not read */
      if (feof(bufd)) return FALSE;   /* finish reading line or file */
    }
  fgets (buffer, BUF_SIZE, bufd); if (feof(bufd)) return FALSE;
  complete = (buffer[strlen(buffer) - 1] == '\n');

  i = strlen(buffer);
  while (i-- && buffer[i]<' ') buffer[i] = '\0';
  return TRUE;
}

static char *newSuffix(char *oldname, char *newsuffix)
{
  char* newstr;

  int c = strrchr(oldname, '.') - oldname;
  if (!c) c = strlen(oldname);

  safeMalloc(newstr, char, c+2+strlen(newsuffix));
  strncpy(newstr, oldname, c+1); newstr[c+1] = '\0';
  strcpy(newstr+c+1, newsuffix);
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
  char *lstFile = NULL;
  char *objFile = NULL;
  int numErr = 0;
  int batch = FALSE;
  int verbose = FALSE;
  int version = FALSE;
  char temp[BUF_SIZE] = "asm51XXXXXX";

  while ((c = getopt(argc, argv, "svhlLo:")) != EOF)
    {
      switch (c)
	{
	case 'l':
	  lstFile = newSuffix(filename, "lst");
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
      if ( (obj = fdopen(mkstemp(temp), "w") ) == NULL)
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

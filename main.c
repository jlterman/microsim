/*************************************************************************************

    Copyright (c) 2003 - 2005 by James L. Terman
    This file is part of the Assembler/Simulator

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
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>

#ifdef __WIN32__
#include <io.h>
#endif

#define MAIN_LOCAL

#include "asmdefs.h"
#include "asm.h"
#include "cpu.h"
#include "err.h"
#include "sim.h"
#include "version.h"

const char asm_version[] = "Assembler " ASM_VERS 
                         "\nBuild date: " BUILD_DATE 
                         "\nCopyright (c) James L. Terman 2003 - 2005\n";

const char sim_version[] = "Assembler " ASM_VERS ", Simulator " SIM_VERS
                         "\nBuild date: " BUILD_DATE 
                         "\nCopyright (c) James L. Terman 2003 - 2005\n";

const str_storage error_messages[LAST_ERR] =
  {
    "no error found",
    /* 
     * expression error messages
     */
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
    "Unknown or illegal register",
    /* 
     * assembler front end error messages
     */
    "Illegal character encountered", 
    "Non-constant label for org directive",
    "Can't use expression with forward defined labels for org directive",
    "Missing colon at end of address label or unrecognized instruction",
    "Bad equ directive statement",
    "Bad db/dw directive statement",
    "Bad character constant",
    "Bad address label",
    "Bad temporary address label",
    "Reference to undefined temporary address label",
    "Cannot use equ to define label defined elsewhere",
    /* 
     * assembler back end  error messages
     */
    "Ambiguous instruction because of forward label definition",
    "Unrecognized instruction",
    "No such instruction",
    "Relative jump out of range",
    "1-bit data constant out of range", 
    "2-bit data constant out of range", 
    "3-bit data constant out of range", 
    "4-bit data constant out of range", 
    "5-bit data constant out of range", 
    "6-bit data constant out of range", 
    "7-bit data constant out of range", 
    "8-bit data constant out of range", 
    "9-bit data constant out of range", 
    "10-bit data constant out of range", 
    "11-bit data constant out of range", 
    "12-bit data constant out of range", 
    "13-bit data constant out of range", 
    "14-bit data constant out of range", 
    "15-bit data constant out of range", 
    "16-bit data constant out of range", 
    "1-bit address out of range", 
    "2-bit address out of range", 
    "3-bit address out of range", 
    "4-bit address out of range", 
    "5-bit address out of range", 
    "6-bit address out of range", 
    "7-bit address out of range", 
    "8-bit address out of range", 
    "9-bit address out of range", 
    "10-bit address out of range", 
    "11-bit address out of range", 
    "12-bit address out of range", 
    "13-bit address out of range", 
    "14-bit address out of range", 
    "15-bit address out of range", 
    "16-bit address out of range", 
    /* 
     * simulator command line error messages
     */
    "Parameter missing from simulator command",
    "Extra parameter given to simulator command",
    "Parameter out of range",
    "Unsupported base",
    "Illegal or out of range opcode",
    "no break at this address",
    "non-existant register",
    "no such display",
    "line already has break",
    "illegal interrupt number",
    "illegal address, must be at start of instruction",
    "bad parameter",
    "cntrl-c interrupted simulation",
    "unknown command",
    /* 
     * simulator execution error messages
     */
    "Program counter has overflowed",
    "Stack has overflowed",
    "Stack has underflowed",
    "Divide by zero exception",
    "Non-existant opcode"
  };

/* Line structure entry containing line and address of line
 */
typedef struct
{
  char *line;
  int pc;
} line_struct;

/* structure to record temp files opened
 */
typedef struct
{
  FILE *fd;
  char *name;
} tmpFILE;

FILE *lst = NULL;                   /* file descriptor for assembly listing */
static char *filename;              /* filename of file being assembled     */
static FILE* obj = NULL;            /* pointer to object file descriptor    */
static line_struct *lines = NULL;   /* hold file to be assembled            */
static tmpFILE  *tmpfiles = NULL;   /* array of all temp file descriptors   */
static int num_files = 0;
static int emacs = FALSE;           /* emacs mode */

/* Global variables just for simulator 
 */
static int asm_Lines[MEMORY_MAX];   /* list of line numbers vs. memory loc */
static char *cmd_store = NULL;      /* storage for simulator command line  */
static char **dsp_list = NULL;      /* array of cmds to be displayed       */
static int num_dsp = 0;             /* number of commands                  */
static int done = FALSE;            /* when true, exit simulator           */

/* variables for outputing correct new line in simulator
 * NEWLNFMT printf format for display command prefix
 * make new line when past LNLNGTH characters
 */
#define NEWLNFMT "[%d] "
#define LNLNGTH 65

static char newLine[7];
static int nchar = 0;

/* defintions for simulator commands
 */
enum simCmd
  {
    brkln, brk_tmp, clr_brk, clr_dsp, go, intr, list_brk, list_dsp,
    next,  print, pr_bin, pr_dec, pr_line, pr_led, pr_reg, pr_hex, 
    quit, resume, resetSim, stpln, trace, lastCmd
  };

/* array of simulator commands strings
 */
#define NUM_SIM 26
static const str_storage simCmdStr[] =
  {
    "b", "break", "bt",    "cb", "cd",    "g",    "i", "lb", "ld",
    "n",  "next",  "p",    "pb", "pd",   "pl", "pled", "pr",
    "px",    "q",  "r", "reset",  "s", "step",    "t", "trace"
  };

static const int simCmd[] =
  {
    brkln, brkln, brk_tmp, clr_brk, clr_dsp, go, intr, list_brk, list_dsp,
    next, next, print, pr_bin, pr_dec, pr_line, pr_led, pr_reg,
    pr_hex, quit, resume, resetSim, stpln, stpln, trace, trace
  };

/* simulator help by letter
 */
static const str_storage sim_help[26] = 
  {
    0, /* a help */
    "b  [line/$addr] [if expr] : break at line# or $addr (pc if not given) if expr is true\n"
    "bt [line/$addr] [if expr] : break will be cleared when hit\n",
    "cb [list] : clear break by number, or all if no params\n"
    "cd [list] : clear display by number or all if no params\n",
    "command will display corresponding print command after each\n"
    "execution of an instruction\n",
    0, 0, /* e, f help */
    "g [line/$addr] : go to line# or $addr and execute (current pc if not given)\n",
    "Simulator help by letter. Type h(letter) for more details\n\n"
    "Types of parameters:\n"
    "line/$addr - line number or with $ memory address\n"
    "addr       - address given by number or value of label\n"
    "$addr      - indirect address\n"
    "@          - get direct address of register\n"
    "expr       - algebraic expression evaluated each time it is used\n"
    "list       - list of numbers or expressions seperated by a space\n"
    "[...]      - optional parameter\n"
    "[repeat]   - optional value to run cmd repeat times\n\n",
    "i expr : request interrupt number expr\n",
    0, 0, /* j, k help */
    "lb [list] : list break number (expr), at addr or all if no param\n"
    "ld [list] : list display number (expr) or all if no param given\n",
    "m [@reg/addr:c] list : assign value(s) to memory or register addr\n"
    "m(c) addr list       : assign value(s) to memory location addr:c\n",
    "n [repeat]  : execute next instr, but skip over subroutine calls\n",
    0, /* o help */
    "p[d] list             : print value of expression in decimal\n"
    "pb   list             : print value of expression in binary\n"
    "pl   [list]           : print asm line at the given line numbers\n"
    "pm[c] addr [length]   : print memory at addr:c\n"
    "pled list             : print value of expression as LED\n"
    "pr   list             : print list of registers (all if no params)\n"
    "px   list             : print expression in hexadecimal\n",
    "q  : quit simmulator\n",
    "r [repeat] : resume execution\n"
    "reset      : reset state of simulator\n",
    "s [repeat] : step one instruction\n",
    "t [repeat] : trace until break\n",
    0, 0, 0, /* u, v, w help */
    0, 0, 0  /* x, y, z help */
  };

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

/* global function called by assembly front end whenever it needs a new line
 */
str_storage getBuffer(void)
{
  static int n = 0;
  if (!lines[n].line)
    { n = 0; return NULL; } /* if NULL line, reset to beginning */
  else
    return lines[n++].line;
}

/* global function called by assembler when error is reported
 */
static void fprintErr(FILE *fd, int errNo, int line)
{
  fprintf(fd, "%s:%d ****** Syntax error #%d: %s", filename, line, errNo, error_messages[errNo]);
  fprintf(fd, "\n%s\n", lines[line-1].line);
}

/* global function called by assembler when error is reported
 */
void printErr(int errNo, int line)
{
  fprintErr(stderr, errNo, line);
  if (lst && lst!=stdout) fprintErr(lst, errNo, line);
}

/* simulator printErr message routine
 */
static void printErrNo(int errNo)
{
  str_storage msg = (errNo < LAST_ERR) ? error_messages[errNo] :  proc_error_messages[errNo - LAST_ERR];
  printf("Simulator Error #%d: %s\n", errNo, msg);
}

/* getNumParam() calls ongoing strtok call to get next number parameter
 * throw error if finalFlag is TRUE but there is an extra parameter
 * return UNDEF if parameter is missing
 */
static int getNumParam(int finalFlag)
{
  char *f, *t = strtok(NULL, "\040\t");
  if (!t || !strcmp("-", t)) return UNDEF;

  if (finalFlag)
    {
      f = strtok(NULL, "\040\t");
      if (f) longjmp(err, extra_param);
    }
  return getExpr(t);
}

/* get address. if '-', return pc, if $ return memory address,
 * otherwise return address of line number given it
 */
static int getAddrParam(int finalFlag)
{
  int addr;
  char *s, *p = strtok(NULL, "\040\t");
  if (!p) return pc;

  if (!strcmp(p, "-"))
    addr = UNDEF;
  else
    {
      if ((s = strchr(p, ':'))) 
	{
	  *s = '\0'; 
	  if (strcmp(p, filename + strlen(filename) - strlen(p)))
	    longjmp(err, bad_param);
	  p = s + 1;
	}
      addr = (p[0]=='$') ? getExpr(p + 1) : lines[getExpr(p) - 1].pc;
      if (asm_Lines[addr] == UNDEF) longjmp(err, sim_addr);
    }

  if (finalFlag)
    {
      p = strtok(NULL, "\040\t");
      if (p) longjmp(err, extra_param);
    }
  return addr;
}

/* getStrParam() calls ongoing strtok call to get next string parameter
 * throw error if errFlag is TRUE and there is no parameter
 * throw error if finalFlag is TRUE but there is an extra parameter
 */
static char *getStrParam(int errFlag, int finalFlag)
{
  char *f, *t = strtok(NULL, "\040\t");
  if (!t && errFlag) longjmp(err, miss_param);
  if (t && finalFlag)
    {
      f = strtok(NULL, "\040\t");
      if (f) longjmp(err, extra_param);
    }
  return t;
}

/* ask for yes or no answer to question
 */
static int answer(str_storage msg)
{
  char buffer[5] = { 0 }; nchar = 0;
  while (strcmp(buffer, "yes\n") && strcmp(buffer, "no\n"))
    {
      printf("%s (yes or no)? ", msg); fflush(stdout);
      fgets(buffer, 5, stdin);
    }
  return (!strcmp(buffer, "yes\n"));
}

/* print this break. Return true if printable, otherwise false
 */
int printOneBreak(FILE* fd, int n, brk_struct* b)
{
  if (b->used)
    {
      if (b->tmp)
	fprintf(fd, "[%2d] Temp ", n);
      else
	fprintf(fd, "[%2d] Permanent ", n);

      fprintf(fd, "break at address $%04X, line %d", b->pc, asm_Lines[b->pc]);
      if (b->expr) fprintf(fd, " if %s", b->expr);
      fprintf(fd, "\n");
      return TRUE;
    }
  else
    return FALSE;
}

/* add break. If bare number, treat as assembly file number.
 * if '$' treat as code address
 */
static void doBrk(int tmpFlag)
{
  char *e = NULL;
  int line, brk, addr = getAddrParam(FALSE);
  if (memory[addr]<0) longjmp(err, dup_brk);

  if ((e = getStrParam(FALSE, FALSE)))
    {
      if (strcmp(e, "if")) longjmp(err, bad_param);
      e = strdup(getStrParam(TRUE, TRUE));
    }
  line = asm_Lines[addr];
  brk  = addBrk(tmpFlag, addr, e);
  nchar += printf("[%2d] %5d %04X: %s", brk, line, addr, lines[line - 1].line);
}

/* if no paramter, ask to clear all breaks. Otherwise clear breaks given
 */
static void doClrBrk()
{
  int brk = getNumParam(FALSE);
  if (brk == UNDEF && answer("Clear all breaks"))
    {
      delBrk(UNDEF); return;
    }
  
  do
    { 
      if (nchar) { printf(newLine); nchar = 0; }
      delBrk(brk);
    }
  while ((brk = getNumParam(FALSE)) != UNDEF);
}

/* if no paramter, ask to clear all display commands
 * Otherwise clear all display commands
 */
static void doClrDsp()
{
  int index = getNumParam(FALSE);
  if (index == UNDEF && answer("Clear all display commands"))
    {
      num_dsp = 0; return;
    }
  
  do
    {
      if (index<1 || index>num_dsp) longjmp(err, out_range);
      --index;
      if (!dsp_list[index]) 
	{ 
	  printf("Warning: No display command #%d\n", index); continue;
	}
      if (nchar) { printf(newLine); nchar = 0; }
      if (dsp_list[index]) nchar = printf(NEWLNFMT "%s", index + 1, dsp_list[index]);

      free(dsp_list[index]);
      dsp_list[index] = NULL;
    }
  while ((index = getNumParam(FALSE)) != UNDEF);
}

/* display takes the list of commands in dsp_list and executes them
 */
static void doCmd(str_storage);

static void display()
{
  int i;
  for (i = 0; i<num_dsp; ++i) 
    {
      if (!dsp_list[i]) continue;
      sprintf(newLine, "\n" NEWLNFMT, i + 1);
      if (nchar)
 	{ 
	  printf(newLine); nchar = 0;
	}
      else
	printf(NEWLNFMT, i + 1);
      doCmd(dsp_list[i]);
    }
}

/* break was hit, print a message to that effect
 */
static void dsp_brk(int line)
{
  if (nchar) putchar('\n');
  nchar = printf("Break at line %d", line);
}

/* every display command has equiv print command.
 * replace 'd' with 'p' char and add to dsp_list
 */
static void doDsp()
{
  static int size_dsp = 0;
  int errNo;
  char *buffer = NULL;

  safeDupStr(buffer, cmd_store);
  buffer[0] = 'p';

  /* to prevent memory leaks, trap any error from trying new display command
   * if error, free new display command and return
   */
  sprintf(newLine, "\n" NEWLNFMT, num_dsp + 1);
  printf(NEWLNFMT, num_dsp + 1); nchar = 0;
  if ((errNo = setjmp(err)) != 0)
    {
      printErrNo(errNo);
      free(buffer);
      return;
    }
  doCmd(buffer);

  safeAddArray(char*, dsp_list, num_dsp, size_dsp);
  dsp_list[num_dsp++] = buffer;
}

/* handle go command. If bare number, treat as assembly file number.
 * if '$' treat as code address
 */
static void doGo()
{
  int addr = run(getAddrParam(TRUE), FALSE);
  if (addr != UNDEF) dsp_brk(asm_Lines[addr]);
  display();
}
/* execute interrupt. Parameter is processor depedent
 */
static void doIRQ()
{
  int irqno = getNumParam(TRUE);
  int result = irq(irqno);
  if (!result) nchar += printf("Interrupt #%d was masked out", irqno);
  if (result == UNDEF) longjmp(err, no_irq);
}

/* list all the breaks by address given to it on command line
 * note: getAddrParam returns pc with empty parameter.
 */
static void doListBrk()
{
  int brk = getNumParam(FALSE);
  do
    {
      printBreak(stdout, brk);
    }
  while ((brk = getNumParam(FALSE)) != UNDEF);
}

/* print the display commands
 */
static void doListDsp()
{
  int d = getNumParam(FALSE);
  if (d == UNDEF)
    {
      for (d = 0; d<num_dsp; ++d) 
	{ 
	  if (!dsp_list[d]) continue;
	  if (nchar) putchar('\n');
	  nchar = printf(NEWLNFMT "%s", d + 1, dsp_list[d]);
	}
      return;
    }

  do
    {
      if (!dsp_list[d]) continue;
      if (nchar) putchar('\n');
      nchar = printf(NEWLNFMT "%s", d, dsp_list[d - 1]);
    }
  while ((d = getNumParam(FALSE)) != UNDEF);
}

/* doMem will set the memory address with the values given it.
 * if more than one value,  they are assigned to succeeding memory address
 */
static void doMem(char c)
{
  char *expr;
  int *mem, addr, value;

  if (c) 
    {
      addr = getNumParam(FALSE);
      if (addr  == UNDEF) longjmp(err, miss_param);
      mem = getMemory(addr, c);
    }
  else
    {
      expr = getStrParam(TRUE, FALSE);
      mem = getMemExpr(expr, &addr, &c);
    }

  value = getNumParam(FALSE);
  if (value == UNDEF) longjmp(err, miss_param);
  do
    {
      if (value<SGN_BYTE_MIN || value>=BYTE_MAX)
	printf("Warning: %s, masked to 8 bits\n", error_messages[out_range]);
      *mem = value & BYTE_MASK; /* if negative, mask out leads 1's */
      ++mem;
    }
  while ((value = getNumParam(FALSE)) != UNDEF);
}

/* doNext will do a step on next instruction. If it is a jump to 
 * subroutine call, set up temp break point after subroutine call.
 */
static void doNext()
{
  int line, brkAddr = UNDEF, addr = pc, repeat = getNumParam(TRUE);
  if (repeat == UNDEF) repeat = 1;
  
  while (repeat--) 
    { 
      if (isJSR(memory[pc]))
	{
	  /* next opcode is a subroutine call
	   * find address of next line of assembly. 
	   */
	  line = asm_Lines[pc] + 1;
	  brkAddr = lines[line - 1].pc;
	  if (memory[lines[line].pc]>0) setNextBrk(brkAddr);
	  addr = run(pc, FALSE); 
	  if (addr != brkAddr) dsp_brk(asm_Lines[addr]);
	}
      else
	stepOne();
      display();
    }

}

/* for each parameter, print it out in the given base
 */
static void doPrintExpr(int base)
{
  char *expr;
  int value, bit, b;

  expr = getStrParam(TRUE, FALSE);
  do
    {
      if (nchar<LNLNGTH) 
	{ 
	  putchar(' '); ++nchar;
	}
      else 
	{ 
	  printf(newLine); nchar = strlen(newLine - 1);
	}
      nchar += printf("%s:", expr);
      value = getExpr(expr);
      if (!value) { printf(" 0"); return; }
      switch (base)
	{
	case 2: 
	  putchar(' '); ++nchar;
	  if (!value) { putchar('0'); ++nchar; }
	  bit=1<<30; while (!(value&bit)) bit /= 2;
	  for (b = bit; b; b /= 2) 
	    { 
	      (value&b) ? putchar('1') : putchar('0'); ++nchar;
	    }
	  break;
	case 10: nchar += printf(" %d", value); break;
	case 16: nchar += printf(" %X", value); break;
	default:
	  longjmp(err, bad_base);
	  break;
	}
    }
  while ((expr = getStrParam(FALSE, FALSE)));
}

/* print each assembly line given to it as a parameter
 */
static void doPrintLine()
{
  char *line;
  int lineNo = getNumParam(FALSE);
  if (lineNo == UNDEF) lineNo = asm_Lines[pc];
  do
    {
      if (nchar) { printf(newLine); nchar = 0; }
      line = lines[lineNo - 1].line;
      nchar = printf("%5d %04X: %s", lineNo, lines[lineNo - 1].pc, lines[lineNo - 1].line);
    }
  while ((lineNo = getNumParam(FALSE)) != UNDEF);
}

/* print each number as if displayed by LED
 */
static void doPrintLED()
{
  int i, b, bits[16] = { 0 }, n = 0;
  
  while (n<16 && (bits[n++] = getNumParam(FALSE)) != UNDEF)
    {
      while (n<16 && (bits[n++] = getNumParam(FALSE)) != UNDEF);

      for (i = 1; i<8; ++i)
	{
	  n = 0;
	  while (n<16 && (b = bits[n++]) != UNDEF)
	    {
	      switch (i)
		{
		case 1: if (b&1)  printf("     "); else printf(" ##  "); break;
		case 4: if (b&64) printf("     "); else printf(" ##  "); break;
		case 7: if (b&8)  printf("     "); else printf(" ##  "); break;
		case 2:
		case 3:
		  if (b&32) printf("   "); else printf("#  ");
		  if (b&2)  printf("  ");  else printf("# ");
		  break;
		case 5:
		case 6:
		  if (b&16) printf("   "); else printf("#  ");
		  if (b&4)  printf("  ");  else printf("# ");
		  break;
		}
	    }
	  printf(newLine); nchar = 0;
	}
    }
}

/* print memory at address for a length of bytes. c determines 
 * which memory area address belongs to
 */
static void doPrintMemory(char c)
{
  int addr, length;
  char *expr, d = (c) ? c : ' ';
  int *mem, i;

  if (c) 
    {
      addr = getNumParam(FALSE);
      if (addr  == UNDEF) longjmp(err, miss_param);
      mem = getMemory(addr, c);
    }
  else
    {
      expr = getStrParam(TRUE, FALSE);
      mem = getMemExpr(expr, &addr, &c);
    }

  length = getNumParam(TRUE);
  if (length == UNDEF) length = 1;
  
  nchar += printf("%04X:%c %02X", addr, d, *mem);
  for (i = 1; i<length; ++i)
    {
      if (!((addr + i)%16)) nchar = printf("%s%04X:%c", newLine, addr+i, d);
      nchar += printf(" %02X", *getMemory(addr + i, c));
    }
}

/* resume execution after break. Note this can be repeated
 */
static void doResume()
{
  int addr, repeat = getNumParam(TRUE);
  if (repeat == UNDEF) repeat = 1;

  while (repeat--) 
    {
      addr = run(pc, FALSE);
      if (addr != UNDEF) dsp_brk(asm_Lines[addr]);
      display();
    }
}

/* execute (s)tep command
 */
static void doStep()
{
  int repeat = getNumParam(TRUE);
  if (repeat == UNDEF) repeat = 1;

  while (repeat--) stepOne();
  display();
}

/*   call back routine for run in sim_run.c to trace exection
 */
void traceDisplay() {
  if (nchar) putchar('\n'); printf("---\n"); nchar = 0;
  display();
}

/* trace execution. Step each instruction and execute display commands
 * until break found
 */
static void doTrace()
{
  int repeat = getNumParam(TRUE);
  if (repeat == UNDEF) repeat = 1;
  
  while (repeat--) run(pc, TRUE);
  if (pc != UNDEF) dsp_brk(asm_Lines[pc]);
}

/* print registers. '-' parameter means print all tokens that are registers
 * otherwise, parameter is passed to getRegister() function
 */
static void doPrintReg()
{
  static char fmt[] = "%s: %0nX ";
  int t, bytes, bit, *reg;
  char *p = getStrParam(FALSE, FALSE);

  if (!p) p = "-";
  do
    {
      if (!strcmp(p, "-"))
	{
	  for (t=0; t<tokens_length; ++t)
	    {
	      if (!(reg = getRegister(tokens[t], &bit, &bytes))) continue;
	      if (nchar>LNLNGTH) { printf(newLine); nchar = 0; }
	      fmt[6] = (bit == UNDEF) ? '0' + 2*bytes : '1';
	      nchar += printf(fmt, tokens[t], (bit == UNDEF) ? *reg : (*reg&bit)>0);
	    }
	}
      else
	{
	  if (nchar>LNLNGTH) { printf(newLine); nchar = 0; }
	  if (!(reg = getRegister(p, &bit, &bytes))) longjmp(err, no_reg);
	  fmt[6] = (bit == UNDEF) ? '0' + 2*bytes : '1';
	  nchar += printf(fmt, p, (bit == UNDEF) ? *reg : (*reg&bit)>0);
	}
    }  while ((p = getStrParam(FALSE, FALSE)));
}

/* doCmd will execute command string given it. Used by command line loop
 * and display command. Note because of token processing, string is copied
 */
static void doCmd(str_storage b)
{
  char *t, *buffer = NULL;
  str_storage *ret = NULL;
  int index;

  safeDupStr(buffer, b);
  t = strtok(buffer, "\040\t");
  ret = bsearch(t, simCmdStr, NUM_SIM, sizeof(str_storage), &cmpstr);
  index = (ret) ? simCmd[ret - simCmdStr] : -1;
  switch (index)
    {
    case brkln:    doBrk(FALSE);       break;
    case brk_tmp:  doBrk(TRUE);        break;
    case clr_brk:  doClrBrk();         break;
    case clr_dsp:  doClrDsp();         break;
    case go:       doGo();             break;
    case intr:     doIRQ();            break;
    case list_brk: doListBrk();        break;
    case list_dsp: doListDsp();        break;
    case next:     doNext();           break;
    case pr_dec:
    case print:    doPrintExpr(10);    break;
    case pr_bin:   doPrintExpr(2);     break;
    case pr_hex:   doPrintExpr(16);    break;
    case pr_line:  doPrintLine();      break;
    case pr_led:   doPrintLED();       break;
    case pr_reg:   doPrintReg();       break;
    case quit:     
      done = answer("Quit simulator"); break;
    case resetSim: reset();            break;
    case resume:   doResume();         break;
    case stpln:    doStep();           break;
    case trace:    doTrace();          break;
    default:
      switch (t[0])
	{
	case 'd':
	  doDsp(cmd_store);
	  break;
	case 'h': 
	  if (!t[1])
	    printf(sim_help['h' - 'a']);
	  else if (sim_help[t[1]-'a'])
	    printf(sim_help[t[1]-'a']);
	  else
	    printf("No help for %c\n", t[1]);
	  break;
	case 'm':
	  doMem(t[1]);
	  break;
	case 'p': 
	  if (t[1] == 'm') 
	    {
	      doPrintMemory(t[2]);
	      break;
	    }
	default : 
	  longjmp(err, no_cmd);
	  break;
	}
      break;
    }
  free(buffer);
}

/* get next line out of file with no buffer contraints
 */
static char *safeGetLine(FILE* fd)
{
  static char buffer[BUFFER_SIZE];
  char *line = NULL;
  
  if (feof(fd)) return NULL;
  buffer[0] = '\0';
  while ((lastChar(buffer) != '\n') && fgets(buffer, BUFFER_SIZE, fd))
    {
      if (!line)
	{
	  safeDupStr(line, buffer);
	}
      else if (cmpstr(line, "\n"))
	{
	  safeRealloc(line, char, strlen(buffer) + strlen(line) + 1);
	  strcat(line, buffer);
	}
    }
  if (line) lastChar(line) = '\0';
  return line;
}

/* fatal error is generated if file can't be opened.
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

/* print command line help for simulator
 */
static void printSimHelp(void)
{
  printf("sim [-q] file.asm\n"
	 "Load the file.asm assembly file and begin simmulating it.\n"
	 "Type 'h' for help inside the simulator\n\n"
	 "    -h    print this message and exit\n"
	 "    -V    print simulator version and exit\n"
	 "    -q    don't print out version info on startup\n"
	 " --asm    Run this program as an assembler. Run 'sim --asm -h' for details\n"
	 "\nProject homepage: http://microsim.sourceforge.net\n"
	 "Send bug reports to jim@termanweb.net\n");
  exit(1);
}

/* load file filename into lines array
 */
static void loadFile(char *filename)
{
  static int size_Lines = 0;          /* max no. of lines in **lines */
  static int num_Lines = 0;           /* no of lines stored */  
  int l;
  FILE *bufd = safeOpen(filename = filename, "r");

  for (l = 0; l < num_Lines; ++l) free(lines[l].line);
  num_Lines = 0;
  do
    {
      safeAddArray(line_struct, lines, num_Lines, size_Lines);
      lines[num_Lines].line = safeGetLine(bufd);
    }
  while (lines[num_Lines++].line);
  fclose(bufd);
}

/* get name for temporary file: $(TMPDIR)/tempXXXXXX
 */
static char *getTmpFile(char *temp)
{
  char *tmpdir, *tmpname = NULL;

#ifdef __WIN32__
  (tmpdir = getenv("TMP")) || (tmpdir = getenv("TEMP")) || 
    (tmpdir = getenv("TMPDIR")) || (tmpdir = ".");
#else
  (tmpdir = getenv("TMPDIR")) || (tmpdir = P_tmpdir) || (tmpdir = ".");
#endif
  safeMalloc(tmpname, char, strlen(tmpdir) + 1 + strlen(temp) + 6 + 1);
  sprintf(tmpname, "%s%c%sXXXXXX", tmpdir, DIRSEP, temp);
  return tmpname;
}

/* open tmp file XXXXXX is replaced by unique ID
 */
static FILE *openTmpFile(char *temp, char *args)
{
  static int size_files = 0;
  FILE *fd;

#ifdef __WIN32__
  mktemp(temp);
  fd = safeOpen(temp, args);
#else
  if ( (fd = fdopen(mkstemp(temp), args) ) == NULL) 
    { /* mkstemp safer for unix systems */
      fprintf(stderr, "Can't open tmp file!\n"); exit(1); 
    }
#endif
  safeAddArray(tmpFILE, tmpfiles, num_files, size_files);
  tmpfiles[num_files].fd = fd;
  tmpfiles[num_files].name = NULL;
  safeDupStr(tmpfiles[num_files].name, temp);
  ++num_files;
  return fd;
}

/* close a tmp file and null out its entry in files
 */
static void safeCloseTmp(FILE *fd, int deleteFlag)
{
  int i = 0;
  while (i<num_files && tmpfiles[i].fd != fd) ++i;
  assert(i<num_files); fclose(fd); 
  if (deleteFlag) remove(tmpfiles[i].name);
  tmpfiles[i].fd = NULL; free(tmpfiles[i].name);
}

/* function to be called on exit
 */
static void removeAllTmp()
{
  int i = 0;
  for (i = 0; i < num_files; ++i) 
    { 
      if (!tmpfiles[i].fd) continue;
      fclose(tmpfiles[i].fd); remove(tmpfiles[i].name);
    }
}


/* will replace the suffix ".xxx" in string oldname with the 
 * new suffix in newsuffix
 */
static char *newSuffix(char *oldname, char *newsuffix)
{
  char* newstr = NULL;

  int c = strrchr(oldname, '.') - oldname;
  if (!c) c = strlen(oldname);

  safeMalloc(newstr, char, c + 2 + strlen(newsuffix));
  strncpy(newstr, oldname, c + 1); newstr[c + 1] = '\0';
  strcpy(newstr + c +1, newsuffix);
  newstr[c] = '.';
  return newstr;
}

/* my signal handler. If cntrl-c trapped and simultion running
 * dump out memory to core file. Otherwise, exit the program gracefully.
 */
void myhandler(int signum)
{
  char *core;
  FILE *fd;

  if (signum != SIGINT && signum != SIGABRT) 
    {
      fprintf(stderr, "A fatal error has occured\n");
    }
  if (!run_sim)
    {
      fprintf(stderr, "Assember was interrupted\n");
    }
  else
    {
      core = newSuffix(filename, "core");
      fd = safeOpen(core, "w");
      dumpMemory(fd);
      fprintf(stderr, "Simulator was interrupted. Memory dumped to %s\n", core);
    }
  exit(1);
}

/* main loop for simulator. Process command lines, load assembly
 * file, assemble it and enter command line loop
 */
int main_sim(int argc, char *argv[])
{
  int c, errNo, i, l, numErr = 0, silent = FALSE;
  unsigned int m;
  char *line, *temp = getTmpFile("sim");
  if (argc<2) printSimHelp();

  for (i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-fullname")) argv[i] = "-f";
    if (!strcmp(argv[i], "-cd")) argv[i] = "-d";
  }

  while ((c = getopt(argc, argv, "qfVhd:")) != EOF)
    {
      switch (c)
	{
	case 'V':
	  printf(sim_version); printf(cpu_version);
	  exit(0);
	  break;
	case 'd':
	  chdir(optarg);
	  break;
	case 'f':
	  emacs = TRUE;
	  break;
	case 'q':
	  silent = TRUE;
	  break;
	case 'h':
	default:
	  for (c = 0; c<argc; ++c) printf("%s ", argv[c]);
	  printf("\n");
	  printSimHelp();
	  break;
	}
    }
  if (optind + 1 > argc) printSimHelp();
  filename = argv[optind];
  loadFile(filename);

  obj = NULL;
  numErr += doPass(&firstPass);
  lst = openTmpFile(temp, "w+");
  numErr += doPass(&secondPass);
  if (numErr) 
    {
      printf("Assembly terminated with %d errors.\n", numErr);
      return (numErr>0);
    }

  rewind(lst);
  for (i = 0; i<MEMORY_MAX; ++i) asm_Lines[i] = UNDEF;
  do 
    {
      i = fscanf(lst, "%5d %04X%*[^\n]\n", &l, &m);
      if (i!=2) continue;
      asm_Lines[m] = l;
      lines[l-1].pc = m;
    } 
  while (!feof(lst));
  safeCloseTmp(lst, TRUE);

  run_sim = TRUE;
  reset();
  if (!silent) 
    {
      printf(sim_version); 
      printf(cpu_version);
      printf("\n");
    }
  printf("Simulating file %s starting at line %d\n", filename, asm_Lines[pc]);
  while (!done)
    {
      if (emacs) printf("\032\032%s:%d:0\n", filename, asm_Lines[pc]);
      strcpy(newLine, "\n");
      if (nchar) { printf("\n"); nchar = 0; }
      printf("> "); fflush(stdout);
      line = safeGetLine(stdin);
      if (line && strlen(line))
	{
	  for (i = 0; line[i]; ++i) line[i] = tolower(line[i]);
	  for (i = 0; line[i] && isspace(line[i]); ++i);
	  free(cmd_store); cmd_store = line + i;
	}

      if ((errNo = setjmp(err)) != 0)
	printErrNo(errNo);
      else
	doCmd(cmd_store);
    }

  remove(temp);
  return 0;
}

/* print command line option for assembler
 */
static void printAsmHelp(void)
{
  printf("asm [-vqlL] [-o file.obj] file1.asm [[-o file2.obj] file2.asm] ...\n"
	 "Assemble the file(s) file1.asm file2.asm ..., writing\n"
	 "the object code to the file(s) file1.obj file2.obj ...\n\n");
  printf("    -V   print version and exit\n"
	 "    -h   print this message and exit\n"
	 "    -v   run in verbose mode\n"
	 "    -q   run in quiet mode - no output to stdout\n"
	 "    -l   save assembly listing output to file.lst\n"
	 "    -L   print assembly listing to stdout\n"
	 "    -o   Set the filename of the object file of the next assembly file\n"
	 "\nProject homepage: http://microsim.sourceforge.net\n"
	 "Send bug reports to jim@termanweb.net\n");
  exit(1);
}

/* main routine for assembler
 */
int main_asm(int argc, char *argv[])
{
  int c;
  int lstFlag = FALSE;
  char *lstFile = NULL;
  char *objFile = NULL;
  int numErr = 0;
  int silent = FALSE;
  int batch = FALSE;
  int verbose = FALSE;
  char temp[] = "asmXXXXXX";

  if (argc<2) printAsmHelp();
  while ((c = getopt(argc, argv, "qvhlVLo:")) != EOF)
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
	  puts(asm_version); puts(cpu_version);
	  exit(0);
	  break;
	case 'v':
	  verbose = TRUE;
	  break;
	case 'q':
	  silent = TRUE;
	  break;
	default:
	  printAsmHelp();
	  break;
  
	}
    }
  if (optind == argc) printAsmHelp();

  for (c = optind; c<argc; ++c)
    {
      if (!strcmp("-o", argv[c]))
	{
	  objFile = argv[c];
	  continue;
	}
      filename = argv[c];
      loadFile(filename);
      if (lstFlag) lstFile = newSuffix(filename, "lst");
      if (batch)
	{
	  numErr += doPass(&firstPass);
	  lst = stdout;
	  numErr += doPass(&secondPass);
	  if (numErr) printf("Assembly terminated with %d errors.\n", numErr);
	  return (numErr>0);
	}
      
      if (!objFile) objFile = newSuffix(filename, "obj");
      obj = openTmpFile(temp, "w");
      
      if (verbose) 
	printf("%s\n%s\nThis program is distributed under the GNU Public License.\n\n", cpu_version, asm_version);
      if (verbose && !strcmp(filename, "-")) printf("Starting assembly from standard input\n");
      if (verbose) printf("Starting assembly of file %s\n", filename);
      
      numErr += doPass(&firstPass);
      if (!numErr && !batch && verbose) printf("First pass successfully completed.\n");
      
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
	  safeCloseTmp(obj, FALSE);
	  rename(temp, objFile);
	  
	  if (verbose) printf("Assembly sucessfully completed.\n"
			      "Object code written to file %s\n", objFile);
	  if (lst) printf("List output written to file %s\n", lstFile);
	}
      if (numErr>0) return numErr;
      objFile = 0;
    }
  return 0;
}

/* call assembler or simulator dependent on argv[0]
 */
int main(int argc, char *argv[])
{
  char *arg0;

#ifdef __WIN32__
  signal(SIGINT, myhandler);
  signal(SIGILL, myhandler);
  signal(SIGABRT, myhandler);
  signal(SIGFPE, myhandler);
  signal(SIGSEGV, myhandler);
  signal(SIGTERM, myhandler);
#else
  struct sigaction sa;

  sa.sa_handler = myhandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGILL, &sa, NULL);
  sigaction(SIGABRT, &sa, NULL);
  sigaction(SIGFPE, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL);
  sigaction(SIGALRM, &sa, NULL);
#endif

  atexit(removeAllTmp);
  /* Look for --asm option to run as assembler. Run sim as default
   */
  if (!strcmp("--asm", argv[1])) return main_asm(argc - 1, argv + 1);

  /* Find program name in argv[0]. If first 3 chars of name after directory seperator 
   * is asm, run as assembler. Run sim for any other name
   */
  arg0 = strrchr(argv[0], DIRSEP);
  arg0 = (arg0) ? arg0 + 1 : argv[0];
  return (strncmp("asm", arg0, 3)) ? main_sim(argc, argv) : main_asm(argc, argv);
}

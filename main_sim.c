/*************************************************************************************

    Copyright (c) 2003, 2004 by James L. Terman
    This file is part of the Simulator

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

extern const char cpu_version[];

extern const char asm_version[];

const char sim_version[] = "Assembler " ASM_VERS ", Simulator " SIM_VERS
                         "\nBuild date: " BUILD_DATE 
                         "\nCopyright (c) James L. Terman 2003, 2004\n";

enum cmd_err
  { 
    miss_param = LAST_SIM_ERR, extra_param, out_range, bad_base, 
    bad_op, no_brk, no_reg, no_dsp, LAST_CMD_ERR
  };

const str_storage cmdErrMsg[LAST_CMD_ERR] =
  {
    "Parameter missing from simulator command",
    "Extra parameter given to simulator command",
    "Parameter out of range",
    "Unsupported base",
    "Illegal or out of range opcode",
    "no break at this address",
    "non-existant register",
    "no such display"
  };

/* Line structure entry containing line and address of line
 */
typedef struct
{
  char *line;
  int pc;
} line_struct;

FILE *lst = NULL;            /* file descriptor for assembly listing */

static char *filename;
static FILE *bufd;                  /* file descriptor for buffer */
static char *cmd_store = NULL;
static char  buf_store[BUFFER_SIZE];
static line_struct *lines = NULL;
static int size_Lines = 0;          /* max no. of lines in **lines */
static int num_Lines = 0;           /* no of lines stored */  
static int asm_Lines[MEMORY_MAX];
static char **dsp_list = NULL;
static int num_dsp = 0;
static int size_dsp = 0;
static int done = FALSE;
static FILE* obj = NULL;

enum simCmd
  {
    brkln, b_help, brk_tmp, clr_brk, clr_dsp, c_help, dsp, dsp_bin, 
    dsp_dec, d_help, dsp_line, dsp_led, dsp_reg, dsp_hex, go, g_help, 
    help, list_brk, list_dsp, l_help, mem, m_help, next, n_help, 
    print, pr_bin, pr_dec, p_help, pr_line, pr_led, pr_reg, pr_hex, 
    quit, q_help, resetSim, resume, r_help, stpln, s_help, lastCmd
  };

static const str_storage simCmdStr[lastCmd] =
  {
    "b",     "bh",    "bt",   "cb",   "cd",   "ch",  "d", "db", 
    "dd",    "dh",    "dl", "dled",   "dr",   "dx",  "g", "gh",
    "h",     "lb",    "ld",   "lh",    "m",   "mh",  "n", "nh",  
    "p",     "pb",    "pd",   "ph",   "pl", "pled", "pr", "px",  
    "q",     "qh", "reset",    "r",   "rh",    "s", "sh"
  };

static const str_storage sim_help[26] = 
  {
    0, /* a help */
    "b  [addr] [if expr] : break at addr (pc if not given) if expr is true\n"
    "bh                  : print break help\n"
    "bt [addr] [if expr] : break will be cleared when hit\n",
    "cb [expr|$addr] : clear break by num, addr, or all if no params\n"
    "cd [list]       : clear display by number or all if no params\n"
    "ch              : print clear help\n",
    "display will add a print command after each next or step\n\n"
    "d[d] list          : display values of list in decimal\n"
    "db   list          : display values of list in binary\n"
    "dh                 : print display help\n"
    "dl   [addr] [expr] : display expr line(s) at addr (pc if not given)\n"
    "dled list          : display values of list as LED\n"
    "dr   [list]        : display list of registers (all if no params)\n"
    "dx   list          : display values of list in hexadecimal\n",
    0, 0, /* e, f help */
    "g [addr] : go to addr and execute (current pc if not given)\n"
    "gh       : print go to help\n",
    "Simulator help by letter. Type (letter)h for more details\n\n"
    "Types of parameters:"
    "addr     - number or expression used as a 16-bit address in code memory\n"
    "$addr    - value of memory stored at the address\n"
    "expr     - algebraic expression evaluated each time it is used\n"
    "list     - list of numbers or expressions seperated by a space\n"
    "[...]    - optional parameter\n"
    "[repeat] - optional value to run cmd repeat times\n\n"
    /*    "b - break at instruction\n"
    "c - clear breaks and display commands\n"
    "d - display expression\n"
    "g - go to address\n"
    "h - help\n"
    "l - list breaks and display commands\n"
    "m - memory set address with expression\n"
    "n - next instruction\n"
    "p - print expression\n"
    "q - quit\n"
    "r - resume execution\n"
    "s - step one instruction\n"*/,
    0, 0, 0, /* i, j, k help */
    "lb [expr|$addr] : list break number (expr), at addr or all if no param\n"
    "ld [expr]       : list display number (expr) or all if no param given\n"
    "lh              : print list help\n",
    "m addr expr     : assign value of memory location addr\n"
    "mh              : print memory help\n",
    "n [repeat]      : execute next instr, but skip over subroutine calls\n"
    "nh              : print next help\n",
    0, /* o help */
    "p[d] list          : print value of expression in decimal\n"
    "pb   list          : print value of expression in binary\n"
    "ph                 : print print help\n"
    "pl   [addr] [expr] : print line at addr (pc if not given)\n"
    "pled list          : print value of expression as LED\n"
    "pr   list          : print list of registers (all if no params)\n"
    "px   expr          : print expression in hexadecimal\n",
    "q  : quit simmulator\n"
    "qh : print quit help\n",
    "r [repeat] : resume execution\n"
    "rh         : print resume help\n\n"
    "reset      : reset state of simulator\n",
    "s [repeat] : step one instruction\n"
    "sh         : print sh help",
    0
  };

/* dummy global function never used by sim */
void writeObj(int curPC, int newPC) {}

/* global function called by assembly front end whenever it needs a new line
 */
str_storage getBuffer(void)
{
  static int n = 0;
  if (n >= num_Lines)
    { n = 0;
      return NULL;
    }
  return lines[n++].line;
}

/* global function called by assembler when error is reported
 */
void printErr(int errNo, int line, str_storage msg)
{
  printf("%s:%d ****** Syntax error #%d: %s\n", filename, line, errNo, msg);
  printf("%s\n", lines[line].line);
}

/* getNum() calls ongoing strtok call to get next number parameter
 * throw error if finalFlag is TRUE but there is an extra parameter
 * return UNDEF if parameter is missing
 */
static int getNum(int finalFlag)
{
  char *f, *t = strtok(NULL, "\040\t");
  if (!t) return UNDEF;

  if (finalFlag)
    {
      f = strtok(NULL, "\040\t");
      if (f) longjmp(err, extra_param);
    }
  return getExpr(t);
}

/* getParam() calls ongoing strtok call to get next string parameter
 * throw error if errFlag is TRUE and there is no parameter
 * throw error if finalFlag is TRUE but there is an extra parameter
 */
static char *getParam(int errFlag, int finalFlag)
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

static void doBrk(int tmpFlag)
{
  int line = getNum(TRUE);
  addBrk(tmpFlag, (line == UNDEF) ? pc : lines[line].pc);
}

static void doClrBrk()
{
  int addr, index;
  char *p = getParam(FALSE, FALSE);
  if (!p)
    {
      buf_store[0] = '\0';
      while (strcmp(buf_store, "y\n") && strcmp(buf_store, "n\n"))
	{
	  printf("Clear all breaks (y/n)?"); fgets(buf_store, BUFFER_SIZE, stdin);
	}
      if (!strcmp(buf_store, "y\n")) delBrk(UNDEF);
      return;
    }
  
  do
    {
      if (p[0]=='$')
	{
	  addr = getExpr(p+1);
	  if (memory[addr]>0) longjmp(err, no_brk);
	  index = -memory[addr];
	}
      else
	index = getExpr(p);
      delBrk(index);
    }
  while ((p = getParam(FALSE, FALSE)));
}

static void doClrDsp()
{
  int index = getNum(FALSE);
  if (index == UNDEF)
    {
      buf_store[0] = '\0';
      while (strcmp(buf_store, "y\n") && strcmp(buf_store, "n\n"))
	{
	  printf("Clear all display commands (y/n)?"); 
	  fgets(buf_store, BUFFER_SIZE, stdin);
	}
      if (!strcmp(buf_store, "y\n")) num_dsp = 0;
      return;
    }
  
  do
    {
      if (index>=num_dsp) longjmp(err, no_dsp);
      free(dsp_list[index]); dsp_list[index] = NULL;
    }
  while ((index = getNum(FALSE)));
}

static void doCmd(str_storage);

static void display()
{
  int i;
  for (i = 0; i<num_dsp; ++i) doCmd(dsp_list[i]);
}

static void doDsp(char *buf)
{
  *buf = 'p';
  if (num_dsp >= size_dsp) 
    safeRealloc(dsp_list, char*, size_dsp += CHUNK_SIZE);
  doCmd(buf);
  dsp_list[num_dsp++] = strdup(buf);
}

static void doGo()
{
  int addr;
  addr = run(getNum(TRUE));
  if (addr != UNDEF) printf("Break at line %d\n", asm_Lines[addr]);
  display();
}

static void doListBrk()
{
  int brk = getNum(FALSE);
  do
    {
      printBreak(stdout, brk);
    }
  while ((brk = getNum(FALSE)) != UNDEF);
}

static void doListDsp()
{
  int i;
  for (i = 0; i<num_dsp; ++i) { if (dsp_list[i]) printf("%2d: %s\n", i, dsp_list[i]); }
}

static void doMem()
{
  int *addr = getMemExpr(getParam(TRUE, FALSE)), mem = getNum(TRUE);
  
  if (mem == UNDEF) longjmp(err, miss_param);
  if (mem<SGN_BYTE_MIN || mem>=BYTE_MAX) longjmp(err, out_range);

  *addr = mem & BYTE_MASK;
}

static void doNext()
{
  int line, addr, repeat = getNum(TRUE);
  if (repeat == UNDEF) repeat = 1;
  
  while (repeat--) 
    { 
      if (isJSR(memory[pc]))
	{
	  /* next opcode is a subroutine call
	   * find address of next line of assembly. 
	   */
	  line = asm_Lines[pc] + 1;
	  addBrk(lines[line].pc, TRUE);
	}
      addr = run(UNDEF); 
      if (addr != UNDEF) printf("Break at line %d\n", asm_Lines[addr]);
      display();
    }

}

static void doPrintExpr(int base)
{
  char *expr;
  int value, bit, b;

  while ((expr = strtok(NULL, "\040\t")))
    {
      value = getExpr(expr);
      switch (base)
	{
	case 2: 
	  putchar(' ');
	  if (!value) putchar('0');
	  bit=1<<31; while (!(value&bit)) bit /= 2;
	  for (b = bit; b; b /= 2) 
	    { 
	      (value&b) ? putchar('1') : putchar('0');
	    }
	  break;
	case 10: printf(" %d", value); break;
	case 16: printf(" %X", value); break;
	default:
	  longjmp(err, bad_base);
	  break;
	}
      putchar('\n');
    }
}

static void doPrintLine()
{
  int n = getNum(FALSE);
  if (n == UNDEF) n = asm_Lines[pc];
  
  do
    {
      printf("%5d %04X: %s\n", n, lines[n-1].pc, lines[n-1].line);
    }
  while ((n = getNum(FALSE)) != UNDEF);
}

static void doPrintLED()
{
  int i, b, bits[16] = { 0 }, n = 0;
  
  while (n<16 && (bits[n++] = getNum(FALSE)) != UNDEF)
    {
      while (n<16 && (bits[n++] = getNum(FALSE)) != UNDEF);

      n = 0;
      for (i = 1; i<8; ++i)
	{
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
	  printf("\n"); n = 0;
	}
    }
}

static int doQuit()
{
  buf_store[0] = '\0';
  while (strcmp(buf_store, "y\n") && strcmp(buf_store, "n\n"))
    {
      printf("Quit simulator (y/n)?"); fgets(buf_store, BUFFER_SIZE, stdin);
    }
  return !strcmp(buf_store, "y\n");
}

static void doResume()
{
  int addr, repeat = getNum(TRUE);
  if (repeat == UNDEF) repeat = 1;

  while (repeat--) 
    {
      addr = run(UNDEF);
      if (addr != UNDEF) printf("Break at line %d\n", asm_Lines[addr]);
      display();
    }
}

static void doStep()
{
  int repeat = getNum(TRUE);
  if (repeat == UNDEF) repeat = 1;

  while (repeat--) stepOne();
  display();
}

static void doPrintReg()
{
  static char fmt[] = "%s: %0nX ";
  int t, bytes, bit, *reg, nchar = 0;
  char *p = getParam(FALSE, FALSE);
  const str_storage *ret;

  if (!p) 
    {
      for (t=0; t<tokens_length; ++t)
	{
	  if ( (bytes = isRegToken(t)) )
	    {
	      reg = getRegister(tokens[t], &bit);
	      fmt[6] = (bit == UNDEF) ? '0' + 2*bytes : '1';
	      nchar += printf(fmt, tokens[t], *reg);
	      if (nchar/75) { putchar('\n'); nchar = 0; }
	    }
	}
      if (nchar) putchar('\n');
      return;
    }

  do
    {
      ret = bsearch(p, tokens, tokens_length, sizeof(str_storage), &cmpstr);
      if (!ret) longjmp(err, no_reg);
      t = ret - tokens;
      if (bytes == isRegToken(t))
	{
	      reg = getRegister(tokens[t], &bit);
	      fmt[6] = (bit == UNDEF) ? '0' + 2*bytes : '1';
	      nchar += printf(fmt, tokens[t], *reg);
	      if (nchar/75) { putchar('\n'); nchar = 0; }
	}
      else
	longjmp(err, no_reg);
    }
  while ((p = getParam(FALSE, FALSE)));
  if (nchar) putchar('\n');
}

static void doCmd(str_storage b)
{
  char *t, *buffer = NULL;
  str_storage *ret;
  int index;

  safeDupStr(buffer, b);
  t = strtok(buffer, "\040\t");
  ret = bsearch(t, simCmdStr, lastCmd, sizeof(str_storage), &cmpstr);
  
  index = ret - simCmdStr;
  switch (index)
    {
    case brkln:    doBrk(FALSE);     break;
    case brk_tmp:  doBrk(TRUE);      break;
    case clr_brk:  doClrBrk();       break;
    case clr_dsp:  doClrDsp();       break;
    case dsp:      case dsp_dec: case dsp_bin: case dsp_line: 
    case dsp_led:  case dsp_reg: case dsp_hex:  
                   doDsp(buffer);    break;
    case go:       doGo();           break;
    case   help: case b_help: case c_help: case d_help: case g_help:
    case l_help: case m_help: case n_help: case p_help: case q_help:
    case r_help: case s_help:
         puts(sim_help[b[0]-'a']);   break;
    case list_brk: doListBrk();      break;
    case list_dsp: doListDsp();      break;
    case mem:      doMem();          break;
    case next:     doNext();         break;
    case pr_dec:
    case print:    doPrintExpr(10);  break;
    case pr_bin:   doPrintExpr(2);   break;
    case pr_hex:   doPrintExpr(16);  break;
    case pr_line:  doPrintLine();    break;
    case pr_led:   doPrintLED();     break;
    case pr_reg:   doPrintReg();     break;
    case quit:     done = doQuit();  break;
    case resetSim: reset();          break;
    case resume:   doResume();       break;
    case stpln:    doStep();         break;
    default:
      printf("Unknown command %s\n", t);
      break;
    }
  free(buffer);
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

static void printHelp(void)
{
  printf("sim file.asm\n");
  printf("sim -h -- print this message\n");
  printf("sim -V -- print simulator version and build date\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  char temp[] = "simXXXXXX";
  int c, errNo, i, l, nextLine, numErr = 0;
  unsigned int m;

  if (!argc) printHelp();
  while ((c = getopt(argc, argv, "Vh")) != EOF)
    {
      switch (c)
	{
	case 'V':
	  puts(sim_version); puts(cpu_version);
	  exit(0);
	  break;
	case 'h':
	default:
	  printHelp();
	  break;
	}
    }
  if (optind + 1 >= argc) printHelp();

  bufd = safeOpen(filename = argv[optind], "r");
  while (fgets(buf_store, BUFFER_SIZE, bufd))
    {
      if (num_Lines >= size_Lines) 
	safeRealloc(lines, line_struct, size_Lines += CHUNK_SIZE);

      nextLine = (lastChar(buf_store) == '\n');
      safeDupStr(lines[num_Lines].line, buf_store);

      while (!nextLine && fgets(buf_store, BUFFER_SIZE, bufd))
	{
	  nextLine = (lastChar(buf_store) == '\n');
	  safeRealloc(lines[num_Lines].line, char, 
		      strlen(lines[num_Lines].line) + strlen(buf_store) + 1);
	  strcat(lines[num_Lines].line, buf_store);
	}
       ++num_Lines;
    }

#ifdef __WIN32__
      mktemp(temp);
      lst = safeOpen(temp, "w+");
#else
      if ( (lst = fdopen(mkstemp(temp), "w+") ) == NULL) 
	{ /* mkstemp safer for unix systems */
	  fprintf(stderr, "Can't open tmp file!\n"); exit(1); 
	}
#endif
  obj = NULL;
  numErr += doPass(&firstPass);
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

  strcpy(buf_store, "\n");
  run_sim = TRUE;
  reset();
  while (!done)
    {
      printf("> "); 
      fgets (buf_store, BUFFER_SIZE, stdin); 
      if (strlen(buf_store))
	{
	  for (i = 0; i < strlen(buf_store); ++i) 
	    buf_store[i] = tolower(buf_store[i]);
	  nextLine = (lastChar(buf_store) == '\n');
	  safeDupStr(lines[num_Lines].line, buf_store);
	  
	  while (!nextLine && fgets(buf_store, BUFFER_SIZE, bufd))
	    {
	      for (i = 0; i < strlen(buf_store); ++i) 
		buf_store[i] = tolower(buf_store[i]);

	      nextLine = (lastChar(buf_store) == '\n');
	      safeRealloc(cmd_store, char, 
			  strlen(cmd_store) + strlen(buf_store) + 1);
	      strcat(cmd_store, buf_store);
	    }
	}

      if ((errNo = setjmp(err)) != 0)
	{
	  assert(errNo >= 0 && errNo< LAST_CMD_ERR);
	  if (errNo < LAST_EXPR_ERR)
	    printf("Syntax Error #%d: %s\n", errNo, exprErrMsg[errNo]);
	  else if (errNo < LAST_SIM_ERR)
	    printf("Syntax Error #%d: %s\n", errNo, exprErrMsg[errNo - LAST_EXPR_ERR]);
	  else
	    printf("Syntax Error #%d: %s\n", errNo, exprErrMsg[errNo - LAST_SIM_ERR]);
	}
      doCmd(cmd_store);
    }

  return 0;
}

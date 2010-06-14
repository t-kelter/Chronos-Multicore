/*
 * modified ptrace.c -  pipeline tracing routines for SS_SMT 1.0 simulator
 *
 * This file was adapted by Ronaldo A. L. Goncalves using 
 * the original ptrace.c file from SimpleScalar 3.0 Tool.
 *
 * Ronaldo is professor at the Informatics Department in the State 
 * University of Maringa - Brazil. 
 *
 * Contacts with him can be done by e_mail: ronaldo@din.uem.br
 * More details about him can be found in http://www.din.uem.br/~ronaldo
 *
 * This work was done as part of his PhD thesis in the SEMPRE project.
 * His advisor was Dr. Philippe Navaux from Federal University of Rio 
 * Grande do Sul - Brazil and contacts with him can be done by e-mail:
 * navaux@inf.ufrgs.br
 * 
 * Also, the professors Mateo Valero and Eduard Ayguade from UPC/Barcelona
 * help Ronaldo to do this simulator, during a technological interchange 
 * accomplished at the labs of the DAC/UPC. Contacts with them can be done
 * by e_mail: mateo@ac.upc.es and eduard@ac.upc.es, respectively.
 *
 * The original ptrace.c file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *  
 * The SimpleScalar suite is currently maintained by Doug Burger and Todd M. Austin.
 * 
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 * SimpleScalar Copyright (C) 1994, 1995, 1996, 1997, 1998 by Todd M. Austin
 *
 * This source file is distributed "as is" in the hope that it will be
 * useful.  The tool set comes with no warranty, and no author or
 * distributor accepts any responsibility for the consequences of its
 * use. 
 * 
 * Everyone is granted permission to copy, modify and redistribute
 * this tool set under the following conditions:
 * 
 *    This source code is distributed for non-commercial use only. 
 *    Please contact the maintainer for restrictions applying to 
 *    commercial use.
 *
 *    Permission is granted to anyone to make or distribute copies
 *    of this source code, either as received or modified, in any
 *    medium, provided that all copyright notices, permission and
 *    nonwarranty notices are preserved, and that the distributor
 *    grants the recipient permission for further redistribution as
 *    permitted by this document.
 *
 *    Permission is granted to distribute this file in compiled
 *    or executable form under the same conditions that apply for
 *    source code, provided that either:
 *
 *    A. it is accompanied by the corresponding machine-readable
 *       source code,
 *    B. it is accompanied by a written offer, with no time limit,
 *       to give anyone a machine-readable copy of the corresponding
 *       source code in return for reimbursement of the cost of
 *       distribution.  This written offer must permit verbatim
 *       duplication by anyone, or
 *    C. it is distributed by someone who received only the
 *       executable form, and is accompanied by a copy of the
 *       written offer of source code that they received concurrently.
 *
 * In other words, you are welcome to use, share and improve this
 * source file.  You are forbidden to forbid anyone else to use, share
 * and improve what you give them.
 *
 * Adaptation 1.0  1999/11/10  Ronaldo A. L. Goncalves
 * The original ptrace.c was adapted to support the ss_smt.c version 1.0.
 * The main adaptations were:
 * 1) Many structures and variables were "replicated" in order to support many
 *    independent pipelines (called slots).
 * 2) Many functions were modified to include the "slot's number" parameter 
 *    passing. 
 * 3) Many function calls were modified to pass the slot's number argument
 *  
 * $Id: ptrace.c,v 1.2 1998/08/27 15:49:17 taustin Exp taustin $
 *
 * $Log: ptrace.c,v $
 * Revision 1.2  1998/08/27 15:49:17  taustin
 * implemented host interface description in host.h
 * added target interface support
 * using new target-dependent myprintf() package
 *
 * Revision 1.1  1997/03/11  01:32:15  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "range.h"
#include "ptrace.h"

/* pipetrace file. See sim_init for inicialization */
FILE *ptrace_outfd[MAX_SLOTS];

/* pipetracing is active. See sim_init for inicialization */
int ptrace_active[MAX_SLOTS];

/* pipetracing range */
struct range_range_t ptrace_range[MAX_SLOTS];

/* one-shot switch for pipetracing. See sim_init for inicialization */
int ptrace_oneshot[MAX_SLOTS];

/* open pipeline trace */
void
ptrace_open(char *fname,		/* output filename */
	    char *range,		/* trace range */
            int sn) /* slot numer */
{
  char *errstr;

  /* parse the output range */
  if (!range)
    {
      /* no range */
      errstr = range_parse_range(":", &ptrace_range[sn],sn);
      if (errstr)
	panic(sn,"cannot parse pipetrace range, use: {<start>}:{<end>}");
      ptrace_active[sn] = TRUE;
    }
  else
    {
      errstr = range_parse_range(range, &ptrace_range[sn],sn);
      if (errstr)
	fatal(sn,"cannot parse pipetrace range, use: {<start>}:{<end>}");
      ptrace_active[sn] = FALSE;
    }

  if (ptrace_range[sn].start.ptype != ptrace_range[sn].end.ptype)
    fatal(sn,"range endpoints are not of the same type");

  /* open output trace file */
  if (!fname || !strcmp(fname, "-") || !strcmp(fname, "stderr"))
    ptrace_outfd[sn] = stderr;
  else if (!strcmp(fname, "stdout"))
    ptrace_outfd[sn] = stdout;
  else
    {
      ptrace_outfd[sn] = fopen(fname, "w");
      if (!ptrace_outfd[sn])
	fatal(sn,"cannot open pipetrace output file `%s'", fname);
    }
}

/* close pipeline trace */
void
ptrace_close(int sn)
{
  if (ptrace_outfd[sn] != NULL && ptrace_outfd[sn] != stderr && ptrace_outfd[sn] != stdout)
    fclose(ptrace_outfd[sn]);
}

/* declare a new instruction */
void
__ptrace_newinst(unsigned int iseq,	/* instruction sequence number */
		 md_inst_t inst,	/* new instruction */
		 md_addr_t pc,		/* program counter of instruction */
		 md_addr_t addr,	/* address referenced, if load/store */
                 int sn)                /* slot number */
{
  myfprintf(sn,ptrace_outfd[sn], "+ %u 0x%08p 0x%08p ", iseq, pc, addr);
  md_print_insn(inst, addr, ptrace_outfd[sn]);
  fprintf(ptrace_outfd[sn], "\n");

  if (ptrace_outfd[sn] == stderr || ptrace_outfd[sn] == stdout)
    fflush(ptrace_outfd[sn]);
}

/* declare a new uop */
void
__ptrace_newuop(unsigned int iseq,	/* instruction sequence number */
		char *uop_desc,		/* new uop description */
		md_addr_t pc,		/* program counter of instruction */
		md_addr_t addr,		/* address referenced, if load/store */
                int sn)                /* slot number */
{
  myfprintf(sn,ptrace_outfd[sn],
	    "+ %u 0x%08p 0x%08p [%s]\n", iseq, pc, addr, uop_desc);

  if (ptrace_outfd[sn] == stderr || ptrace_outfd[sn] == stdout)
    fflush(ptrace_outfd[sn]);
}

/* declare instruction retirement or squash */
void
__ptrace_endinst(unsigned int iseq,	/* instruction sequence number */
                 int sn)                /* slot number */
{
  fprintf(ptrace_outfd[sn], "- %u\n", iseq);

  if (ptrace_outfd[sn] == stderr || ptrace_outfd[sn] == stdout)
    fflush(ptrace_outfd[sn]);
}

/* declare a new cycle */
void
__ptrace_newcycle(tick_t cycle,		/* new cycle */
                  int sn)               /* slot number */
{
  fprintf(ptrace_outfd[sn], "@ %.0f\n", (double)cycle);

  if (ptrace_outfd[sn] == stderr || ptrace_outfd[sn] == stdout)
    fflush(ptrace_outfd[sn]);
}

/* indicate instruction transition to a new pipeline stage */
void
__ptrace_newstage(unsigned int iseq,	/* instruction sequence number */
		  char *pstage,		/* pipeline stage entered */
		  unsigned int pevents, /* pipeline events while in stage */
                  int sn)               /* slot number */
{
  fprintf(ptrace_outfd[sn], "* %u %s 0x%08x\n", iseq, pstage, pevents);

  if (ptrace_outfd[sn] == stderr || ptrace_outfd[sn] == stdout)
    fflush(ptrace_outfd[sn]);
}

/*
 * modified ptrace.h -  pipeline tracing routines for SS_SMT 1.0 simulator
 *
 * This file was adapted by Ronaldo A. L. Goncalves using 
 * the original ptrace.h file from SimpleScalar 3.0 Tool.
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
 * The original ptrace.h file is a part of the SimpleScalar tool suite written by
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
 * The original ptrace.h was adapted to support the ss_smt.c version 1.0.
 * The main adaptations were:
 * 1) Many structures and variables were "replicated" in order to support many
 *    independent pipelines (called slots).
 * 2) Many functions were modified to include the "slot's number" parameter 
 *    passing. 
 * 3) Many function calls were modified to pass the slot's number argument
 * 
 * $Id: ptrace.h,v 1.2 1998/08/27 15:50:04 taustin Exp taustin $
 *
 * $Log: ptrace.h,v $
 * Revision 1.2  1998/08/27 15:50:04  taustin
 * implemented host interface description in host.h
 * added target interface support
 *
 * Revision 1.1  1997/03/11  01:32:28  taustin
 * Initial revision
 *
 *
 */

#ifndef PTRACE_H
#define PTRACE_H

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "range.h"

/*
 * pipeline events:
 *
 *	+ <iseq> <pc> <addr> <inst>	- new instruction def
 *	- <iseq>			- instruction squashed or retired
 *	@ <cycle>			- new cycle def
 *	* <iseq> <stage> <events>	- instruction stage transition
 *
 */

/*
	[IF]   [DA]   [EX]   [WB]   [CT]
         aa     dd     jj     ll     nn
         bb     ee     kk     mm     oo
         cc                          pp
 */

/* pipeline stages */
#define PST_IFETCH		"IF"
#define PST_DISPATCH		"DA"
#define PST_EXECUTE		"EX"
#define PST_WRITEBACK		"WB"
#define PST_COMMIT		"CT"

/* pipeline events */
#define PEV_CACHEMISS		0x00000001	/* I/D-cache miss */
#define PEV_TLBMISS		0x00000002	/* I/D-tlb miss */
#define PEV_MPOCCURED		0x00000004	/* mis-pred branch occurred */
#define PEV_MPDETECT		0x00000008	/* mis-pred branch detected */
#define PEV_AGEN		0x00000010	/* address generation */

/* pipetrace file */
extern FILE *ptrace_outfd[MAX_SLOTS];

/* pipetracing is active */
extern int ptrace_active[MAX_SLOTS];

/* pipetracing range */
extern struct range_range_t ptrace_range[MAX_SLOTS];

/* one-shot switch for pipetracing */
extern int ptrace_oneshot[MAX_SLOTS];

/* open pipeline trace */
void
ptrace_open(char *range,		/* trace range */
	    char *fname,		/* output filename */
            int sn);                    /* slot number */

/* close pipeline trace */
void
ptrace_close(int sn);

/* NOTE: pipetracing is a one-shot switch, since turning on a trace more than
   once will mess up the pipetrace viewer */
#define ptrace_check_active(PC, ICNT, CYCLE)				\
  ((ptrace_outfd[sn] != NULL						\
    && !range_cmp_range1(&ptrace_range[sn], (PC), (ICNT), (CYCLE),sn))		\
   ? (!ptrace_oneshot[sn] ? (ptrace_active[sn] = ptrace_oneshot[sn] = TRUE) : FALSE)\
   : (ptrace_active[sn] = FALSE))

/* main interfaces, with fast checks */
#define ptrace_newinst(A,B,C,D)						\
  if (ptrace_active[sn]) __ptrace_newinst((A),(B),(C),(D),sn)
#define ptrace_newuop(A,B,C,D)						\
  if (ptrace_active[sn]) __ptrace_newuop((A),(B),(C),(D),sn)
#define ptrace_endinst(A)						\
  if (ptrace_active[sn]) __ptrace_endinst((A),sn)
#define ptrace_newcycle(A)						\
  if (ptrace_active[sn]) __ptrace_newcycle((A),sn)
#define ptrace_newstage(A,B,C)						\
  if (ptrace_active[sn]) __ptrace_newstage((A),(B),(C),sn)

#define ptrace_active(A,I,C)						\
  (ptrace_outfd[sn] != NULL	&& !range_cmp_range(&ptrace_range[sn], (A), (I), (C), sn))

/* declare a new instruction */
void
__ptrace_newinst(unsigned int iseq,	/* instruction sequence number */
		 md_inst_t inst,	/* new instruction */
		 md_addr_t pc,		/* program counter of instruction */
		 md_addr_t addr, 	/* address referenced, if load/store */
                 int sn);               /* slot number */

/* declare a new uop */
void
__ptrace_newuop(unsigned int iseq,	/* instruction sequence number */
		char *uop_desc,		/* new uop description */
		md_addr_t pc,		/* program counter of instruction */
		md_addr_t addr, 	/* address referenced, if load/store */
                int sn);               /* slot number */

/* declare instruction retirement or squash */
void
__ptrace_endinst(unsigned int iseq,	/* instruction sequence number */
                 int sn);               /* slot number */

/* declare a new cycle */
void
__ptrace_newcycle(tick_t cycle,		/* new cycle */
                  int sn);              /* slot number */

/* indicate instruction transition to a new pipeline stage */
void
__ptrace_newstage(unsigned int iseq,	/* instruction sequence number */
		  char *pstage,		/* pipeline stage entered */
		  unsigned int pevents, /* pipeline events while in stage */
                  int sn);              /* slot number */

#endif /* PTRACE_H */

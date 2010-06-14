/*
 * modified dlite.c - DLite, the lite debugger, routines for 
 *                    SS_SMT 1.0 simulator
 *
 * This file was adapted by Ronaldo A. L. Goncalves using 
 * the original dlite.h file from SimpleScalar 3.0 Tool.
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
 * The original dlite.h file is a part of the SimpleScalar tool suite written by
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
 * The original dlite.h was adapted to support the ss_smt.c version 1.0.
 * The main adaptations were:
 * 1) Many structures and variables were "replicated" in order to support many
 *    independent pipelines (called slots).
 * 2) Many functions were modified to include the "slot's number" parameter 
 *    passing. 
 * 3) Many function calls were modified to pass the slot's number argument
 *
 * $Id: dlite.h,v 1.2 1998/08/27 08:19:44 taustin Exp $
 *
 * $Log: dlite.h,v $
 * Revision 1.2  1998/08/27 08:19:44  taustin
 * implemented host interface description in host.h
 * added target interface support
 * added support for quadword's
 * added support for register and memory contexts
 *
 * Revision 1.1  1997/03/11  01:30:26  taustin
 * Initial revision
 *
 *
 */

/*
 * This module implements DLite, the lite debugger.  DLite is a very light
 * weight semi-symbolic debbugger that can interface to any simulator with
 * only a few function calls.  See sim-safe.c for an example of how to
 * interface DLite to a simulator.
 *
 * The following commands are supported by DLite: 
 *

 *
 * help			 - print command reference
 * version		 - print DLite version information
 * terminate		 - terminate the simulation with statistics
 * quit			 - exit the simulator
 * cont {<addr>}	 - continue program execution (optionally at <addr>)
 * step			 - step program one instruction
 * next			 - step program one instruction in current procedure
 * print <expr>		 - print the value of <expr>
 * regs			 - print register contents
 * mstate		 - print machine specific state (simulator dependent)
 * display/<mods> <addr> - display the value at <addr> using format <modifiers>
 * dump {<addr>} {<cnt>} - dump memory at <addr> (optionally for <cnt> words)
 * dis <addr> {<cnt>}	 - disassemble instructions at <addr> (for <cnt> insts)
 * break <addr>		 - set breakpoint at <addr>, returns <id> of breakpoint
 * dbreak <addr> {r|w|x} - set data breakpoint at <addr> (for (r)ead, (w)rite,
 *			   and/or e(x)ecute, returns <id> of breakpoint
 * breaks		 - list active code and data breakpoints
 * delete <id>		 - delete breakpoint <id>
 * clear		 - clear all breakpoints (code and data)
 *
 * ** command args <addr>, <cnt>, <expr>, and <id> are any legal expression:
 *
 * <expr>		<- <factor> +|- <expr>
 * <factor>		<- <term> *|/ <factor>
 * <term>		<- ( <expr> )
 *			   | - <term>
 *			   | <const>
 *			   | <symbol>
 *			   | <file:loc>
 *
 * ** command modifiers <mods> are any of the following:
 *
 * b - print a byte
 * h - print a half (short)
 * w - print a word
 * q - print a quadword
 * t - print in decimal format
 * u - print in unsigned decimal format
 * o - print in octal format
 * x - print in hex format
 * 1 - print in binary format
 * f - print a float
 * d - print a double
 * c - print a character
 * s - print a string
 */

#ifndef DLITE_H
#define DLITE_H

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "eval.h"

/* DLite register access function, the debugger uses this function to access
   simulator register state */
typedef char *					/* error str, NULL if none */
(*dlite_reg_obj_t)(struct regs_t *regs,		/* registers to access */
		   int is_write,		/* access type */
		   enum md_reg_type rt,		/* reg bank to access */
		   int reg,			/* register number */
		   struct eval_value_t *val,	/* input, output */
                   int sn);                     /* slot number */

/* DLite memory access function, the debugger uses this function to access
   simulator memory state */
typedef char *					/* error str, NULL if none */
(*dlite_mem_obj_t)(struct mem_t *mem,		/* memory space to access */
		   int is_write,		/* access type */
		   md_addr_t addr,		/* address to access */
		   char *p,			/* input/output buffer */
		   int nbytes,			/* size of access */
                   int sn);                     /* slot number */

/* DLite memory access function, the debugger uses this function to display
   the state of machine-specific state */
typedef char *					/* error str, NULL if none */
(*dlite_mstate_obj_t)(FILE *stream,		/* output stream */
		      char *cmd,		/* optional command string */
		      struct regs_t *regs,	/* registers to access */
		      struct mem_t *mem,	/* memory space to access */
                      int sn);                  /* slot number */

/* initialize the DLite debugger */
void
dlite_init(dlite_reg_obj_t reg_obj,		/* register state object */
	   dlite_mem_obj_t mem_obj,		/* memory state object */
	   dlite_mstate_obj_t mstate_obj);	/* machine state object */

/*
 * default architected/machine state accessors
 */

/* default architected memory state accessor */
char *						/* err str, NULL for no err */
dlite_mem_obj(struct mem_t *mem,		/* memory space to access */
	      int is_write,			/* access type */
	      md_addr_t addr,			/* address to access */
	      char *p,				/* input, output */
	      int nbytes,			/* size of access */
              int sn);                          /* slot number */

/* default architected machine-specific state accessor */
char *						/* err str, NULL for no err */
dlite_mstate_obj(FILE *stream,			/* output stream */
		 char *cmd,			/* optional command string */
		 struct regs_t *regs,		/* registers to access */
		 struct mem_t *mem,		/* memory space to access */
                 int sn);                       /* slot number */

/* state access masks */
#define ACCESS_READ	0x01			/* read access allowed */
#define ACCESS_WRITE	0x02			/* write access allowed */
#define ACCESS_EXEC	0x04			/* execute access allowed */

/* non-zero iff one breakpoint is set, for fast break check */
extern md_addr_t dlite_fastbreak /* = 0 */;

/* set non-zero to enter DLite after next instruction */
extern int dlite_active[MAX_SLOTS] /* = FALSE */;
extern int dlite_initial;

/* non-zero to force a check for a break */
extern int dlite_check[MAX_SLOTS] /* = FALSE */;

/* internal break check interface */
int						/* non-zero if brkpt hit */
__check_break(md_addr_t next_PC,		/* address of next inst */
	      int access,			/* mem access of last inst */
	      md_addr_t addr,			/* mem addr of last inst */
	      counter_t icount,			/* instruction count */
	      counter_t cycle,			/* cycle count */
              int sn);

/* check for a break condition */
#define dlite_check_break(NPC, ACCESS, ADDR, ICNT, CYCLE)		\
  ((dlite_check[sn] || dlite_active[sn])				\
   ? __check_break((NPC), (ACCESS), (ADDR), (ICNT), (CYCLE),sn)		\
   : FALSE)

/* DLite debugger main loop */
void
dlite_main(md_addr_t regs_PC,			/* addr of last inst to exec */
	   md_addr_t next_PC,			/* addr of next inst to exec */
	   counter_t cycle,			/* current processor cycle */
	   struct regs_t *regs,			/* registers to access */
	   struct mem_t *mem,			/* memory to access */
           int sn);                             /* slot number */

#endif /* DLITE_H */

/*
 * modified symbol.h - program symbol and line data routines for SS_SMT 1.0 simulator
 *
 * This file was adapted by Ronaldo A. L. Goncalves using 
 * the original routines.h file from SimpleScalar 3.0 Tool.
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

 * The original routines.h file is a part of the SimpleScalar tool suite written by
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
 * The original symbol.h was adapted to support the ss_smt.c version 1.0.
 * The main adaptations were:
 * 1) Many structures and variables were "replicated" in order to support many
 *    independent pipelines (called slots).
 * 2) Many functions were modified to include the "slot's number" parameter 
 *    passing. 
 * 3) Many function calls were modified to pass the slot's number argument
 * 
 * $Id: symbol.h,v 1.2 1998/08/27 16:45:17 taustin Exp taustin $
 *
 * $Log: symbol.h,v $
 * Revision 1.2  1998/08/27 16:45:17  taustin
 * implemented host interface description in host.h
 * added target interface support
 * moved target-dependent definitions to target files
 * added support for register and memory contexts
 *
 * Revision 1.1  1997/03/11  01:34:37  taustin
 * Initial revision
 *
 *
 */

#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "machine.h"

enum sym_seg_t {
  ss_data,			/* data segment symbol */
  ss_text,			/* text segment symbol */
  ss_NUM
};

/* internal program symbol format */
struct sym_sym_t {
  char *name;			/* symbol name */
  enum sym_seg_t seg;		/* symbol segment */
  int initialized;		/* initialized? (if data segment) */
  int pub;			/* externally visible? */
  int local;			/* compiler local symbol? */
  md_addr_t addr;		/* symbol address value */
  int size;			/* bytes to next symbol */
};

/* symbol database in no particular order */
extern struct sym_sym_t *sym_db[MAX_SLOTS];

/* all symbol sorted by address */
extern int sym_nsyms[MAX_SLOTS];
extern struct sym_sym_t **sym_syms[MAX_SLOTS];

/* all symbols sorted by name */
extern struct sym_sym_t **sym_syms_by_name[MAX_SLOTS];

/* text symbols sorted by address */
extern int sym_ntextsyms[MAX_SLOTS];
extern struct sym_sym_t **sym_textsyms[MAX_SLOTS];

/* text symbols sorted by name */
extern struct sym_sym_t **sym_textsyms_by_name[MAX_SLOTS];

/* data symbols sorted by address */
extern int sym_ndatasyms[MAX_SLOTS];
extern struct sym_sym_t **sym_datasyms[MAX_SLOTS];

/* data symbols sorted by name */
extern struct sym_sym_t **sym_datasyms_by_name[MAX_SLOTS];

/* load symbols out of FNAME */
void
sym_loadsyms(char *fname,		/* file name containing symbols */
	     int load_locals,		/* load local symbols */
             int sn);

/* dump symbol SYM to output stream FD */
void
sym_dumpsym(struct sym_sym_t *sym,	/* symbol to display */
	    FILE *fd);			/* output stream */

/* dump all symbols to output stream FD */
void
sym_dumpsyms(FILE *fd, int sn);			/* output stream */

/* dump all symbol state to output stream FD */
void
sym_dumpstate(FILE *fd, int sn);		/* output stream */

/* symbol databases available */
enum sym_db_t {
  sdb_any,			/* search all symbols */
  sdb_text,			/* search text symbols */
  sdb_data,			/* search data symbols */
  sdb_NUM
};

/* bind address ADDR to a symbol in symbol database DB, the address must
   match exactly if EXACT is non-zero, the index of the symbol in the
   requested symbol database is returned in *PINDEX if the pointer is
   non-NULL */
struct sym_sym_t *			/* symbol found, or NULL */
sym_bind_addr(md_addr_t addr,		/* address of symbol to locate */
	      int *pindex,		/* ptr to index result var */
	      int exact,		/* require exact address match? */
	      enum sym_db_t db, 	/* symbol database to search */
              int sn);                  /* slot number */

/* bind name NAME to a symbol in symbol database DB, the index of the symbol
   in the requested symbol database is returned in *PINDEX if the pointer is
   non-NULL */
struct sym_sym_t *				/* symbol found, or NULL */
sym_bind_name(char *name,			/* symbol name to locate */
	      int *pindex,			/* ptr to index result var */
	      enum sym_db_t db,		/* symbol database to search */
              int sn);                  /* slot number */


/* to intialize the replicated global variables from sim_init */

void symbol_init_declarations(void);

#endif /* SYMBOL_H */


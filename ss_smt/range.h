/*
 * modified range.h -  program execution range routines for SS_SMT 1.0 simulator
 *
 * This file was adapted by Ronaldo A. L. Goncalves using 
 * the original range.h file from SimpleScalar 3.0 Tool.
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
 * The original range.h file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *  
 * The SimpleScalar suite is currently maintained by Doug Burger and Todd M. Austin.
 *
 * Adaptation 1.0  1999/11/10  Ronaldo A. L. Goncalves
 * The original range.h was adapted to support the ss_smt.c version 1.0.
 * The main adaptations were:
 * 1) Many functions were modified to include the "slot's number" parameter 
 *    passing. 
 * 2) Many function calls were modified to pass the slot's number argument
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
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 * $Id: range.h,v 1.2 1998/08/27 15:50:50 taustin Exp taustin $
 *
 * $Log: range.h,v $
 * Revision 1.2  1998/08/27 15:50:50  taustin
 * implemented host interface description in host.h
 * added target interface support
 *
 * Revision 1.1  1997/03/11  01:32:44  taustin
 * Initial revision
 *
 *
 */

#ifndef RANGE_H
#define RANGE_H

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "machine.h"

enum range_ptype_t {
  pt_addr = 0,			/* address position */
  pt_inst,			/* instruction count position */
  pt_cycle,			/* cycle count position */
  pt_NUM
};

/*
 * an execution position
 *
 *   by addr:		@<addr>
 *   by inst count:	<icnt>
 *   by cycle count:	#<cycle>
 *
 */
struct range_pos_t {
  enum range_ptype_t ptype;	/* type of position */
  counter_t pos;		/* position */
};

/* an execution range */
struct range_range_t {
  struct range_pos_t start;
  struct range_pos_t end;
};

/* parse execution position *PSTR to *POS */
char *						/* error string, or NULL */
range_parse_pos(char *pstr,			/* execution position string */
		struct range_pos_t *pos,	/* position return buffer */
                int sn);

/* print execution position *POS */
void
range_print_pos(struct range_pos_t *pos,	/* execution position */
		FILE *stream,			/* output stream */
                int sn);                        /* slot number */

/* parse execution range *RSTR to *RANGE */
char *						/* error string, or NULL */
range_parse_range(char *rstr,			/* execution range string */
		  struct range_range_t *range,	/* range return buffer */
                  int sn);                      /* slot number */

/* print execution range *RANGE */
void
range_print_range(struct range_range_t *range,	/* execution range */
		  FILE *stream,		/* output stream */
                  int sn);              /* slot number */

/* determine if inputs match execution position */
int						/* relation to position */
range_cmp_pos(struct range_pos_t *pos,		/* execution position */
	      counter_t val);			/* position value */

/* determine if inputs are in range */
int						/* relation to range */
range_cmp_range(struct range_range_t *range,	/* execution range */
		counter_t val,			/* position value */
                int sn);                        /* slot number */


/* determine if inputs are in range, passes all possible info needed */
int						/* relation to range */
range_cmp_range1(struct range_range_t *range,	/* execution range */
		 md_addr_t addr,		/* address value */
		 counter_t icount,		/* instruction count */
		 counter_t cycle,		/* cycle count */
                 int sn);                       /* slot number */


/*
 *
 * <range> := {<start_val>}:{<end>}
 * <end>   := <end_val>
 *            | +<delta>
 *
 */

#endif /* RANGE_H */

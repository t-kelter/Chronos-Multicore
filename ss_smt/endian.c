/*
 * modified endian.c - host endian probes for SS_SMT 1.0 simulator
 *
 * This file was adapted by Ronaldo A. L. Goncalves using 
 * the original endian.c file from SimpleScalar 3.0 Tool.
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

 * The original endian.c file is a part of the SimpleScalar tool suite written by
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
 * The original endian.c was adapted to support the ss_smt.c version 1.0.
 * The main adaptations were:
 * 1) Many functions were modified to include the "slot's number" parameter 
 *    passing. 
 * 2) Many function calls were modified to pass the slot's number argument
 *
 * $Id: endian.c,v 1.5 1998/08/27 08:23:44 taustin Exp $
 *
 * $Log: endian.c,v $
 * Revision 1.5  1998/08/27 08:23:44  taustin
 * implemented host interface description in host.h
 *
 * Revision 1.4  1997/03/11  01:10:18  taustin
 * updated copyright
 *
 * Revision 1.3  1997/01/06  15:58:32  taustin
 * comments updated
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include "endian.h"
#ifndef HOST_ONLY
#include "loader.h"
#endif

/* probe host (simulator) byte endian format */
enum endian_t
endian_host_byte_order(int sn)
{
  int i = 1, *p;

  p = &i;
  if (*((char *)p) == 1)
    return endian_little;
  else if (*((char *)p) == 0)
    return endian_big;
  else
    return endian_unknown;
}

/* probe host (simulator) double word endian format */
enum endian_t
endian_host_word_order(void)
{
  int *p;
  double x = 1.0;

  /* NOTE: this check assumes IEEE floating point format */
  p = (int *)&x;
  if (*p == 0)
    return endian_little;
  else if (*p == 0x3ff00000)
    return endian_big;
  else
    return endian_unknown;
}

#ifndef HOST_ONLY

/* probe target (simulated program) byte endian format, only
   valid after program has been loaded */
enum endian_t
endian_target_byte_order(int sn)
{
  return ld_target_big_endian[sn] ? endian_big : endian_little;
}

/* probe target (simulated program) double word endian format,
   only valid after program has been loaded */
enum endian_t
endian_target_word_order(int sn)
{
  /* same as byte sex for SimpleScalar target */
  return endian_target_byte_order(sn);
}

#endif /* !HOST_ONLY */
